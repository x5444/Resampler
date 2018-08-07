library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library RSMPL;
use RSMPL.RSMPL_PKG.all;
use RSMPL.RSMPL_COMP_PKG.all;


entity RSMPL_TOP is
	port (
		CLK : in  std_logic;
		
		DATA_IN : in  signed (ADC_N_BITS-1 downto 0);
		
		DATA_OUT    : out signed (ADC_N_BITS-1 downto 0);
		DATA_OUT_EN : out std_logic;
		
		CONF_RSMPL_FACTOR : in  unsigned (RSMPL_FACTOR_N_BITS-1 downto 0)
	);
end entity;


architecture RTL of RSMPL_TOP is
	
	constant N_PIPELINE_STAGES : integer := 5;
	
	type   SAMPLE_BUFFER_ELEMENT_t      is array (0 to C_N_COEFFICIENTS-1)  of signed (ADC_N_BITS-1 downto 0);
	type   SAMPLE_BUFFER_t              is array (0 to N_PIPELINE_STAGES-1) of SAMPLE_BUFFER_ELEMENT_t;
	signal SAMPLE_BUFFER                : SAMPLE_BUFFER_t;
	
	type   COEFFICIENT_BUFFER_ELEMENT_t is array (0 to C_N_COEFFICIENTS-1)  of signed (C_COEFFICIENT_N_BITS-1 downto 0);
	type   COEFFICIENT_BUFFER_t         is array (0 to N_PIPELINE_STAGES-1) of COEFFICIENT_BUFFER_ELEMENT_t;
	signal RIGHT_COEFFICIENTS           : COEFFICIENT_BUFFER_t;
	signal LEFT_COEFFICIENTS            : COEFFICIENT_BUFFER_t;
	
	signal NEXT_SAMPLE_POS   : unsigned (RSMPL_FACTOR_N_BITS-1            downto 0);
	signal  CUR_SAMPLE_POS_P : unsigned (RSMPL_FACTOR_FRACTIONAL_N_BITS-1 downto 0);
	signal  CUR_SAMPLE_POS_N : unsigned (RSMPL_FACTOR_FRACTIONAL_N_BITS-1 downto 0);
	
	signal ENABLE_OUTPUT : std_logic;
	
begin
	
	
	-- Move input data into the sample buffer.
	--
	p_sampleBuffer : process (CLK)
	begin
		if rising_edge(CLK) then
			SAMPLE_BUFFER(0)(1 to SAMPLE_BUFFER(0)'high) <= SAMPLE_BUFFER(0)(0 to SAMPLE_BUFFER(0)'high-1);
			SAMPLE_BUFFER(0)(0)                          <= DATA_IN;
		end if;
	end process;
	
	
	-- Process to calculate the position of the next output sample.
	--
	p_samplePosCalc : process (CLK)
	begin
		if rising_edge(CLK) then
			
			-- Each cyclic is C_STEP_WIDTH steps. In case we need to wait for more
			-- than a step width, we won't output anything.
			--
			if NEXT_SAMPLE_POS >= C_STEP_WIDTH then
				CUR_SAMPLE_POS_P <= (others => '0');
				CUR_SAMPLE_POS_N <= (others => '0');
				NEXT_SAMPLE_POS  <= NEXT_SAMPLE_POS - C_STEP_WIDTH;
				ENABLE_OUTPUT    <= '0';
				
			-- If it's less than a step width, we need to start doing some
			-- calculations.
			--
			else
				assert NEXT_SAMPLE_POS < 2**CUR_SAMPLE_POS_P'high report "Error: out of range" severity FAILURE;
				CUR_SAMPLE_POS_P <=                NEXT_SAMPLE_POS (CUR_SAMPLE_POS_P'high downto 0);
				CUR_SAMPLE_POS_N <= C_STEP_WIDTH - NEXT_SAMPLE_POS (CUR_SAMPLE_POS_N'high downto 0);
				NEXT_SAMPLE_POS  <= CUR_SAMPLE_POS_P + CONF_RSMPL_FACTOR - C_STEP_WIDTH;
				ENABLE_OUTPUT    <= '1';
			end if;
		end if;
	end process;
	
	
	inst_FristProcessingUnit : RSMPL_PROCESSING_UNIT
			generic map (
				USE_EARLY_EN => '1'
			)
			port map (
				CLK         => CLK,
				EARLY_EN    => ENABLE_OUTPUT,
				LATE_EN     => '0',
				SAPMLE_POS  => CUR_SAMPLE_POS_P,
				COEFF_LEFT  => LEFT_COEFFICIENTS( 0)(0),
				COEFF_RIGHT => RIGHT_COEFFICIENTS(0)(0),
				SAMPLE      => SAMPLE_BUFFER(0)(0),
				CARRY_IN    => (others => '0'),
				RESULT_OUT  => RESULT(0),
				ENABLE_OUT  => PU_ENABLE_OUT(0)
			);
	
	
	gen_ProcessingUnits : for k_ProcessingUnit in 1 to C_N_COEFFICIENTS-1 generate
		inst_ProcessingUnit : RSMPL_PROCESSING_UNIT
			generic map (
				USE_EARLY_EN => '0'
			)
			port map (
				CLK         => CLK,
				EARLY_EN    => '0',
				LATE_EN     => PU_ENABLE_OUT(k_ProcessingUnit-1),
				SAPMLE_POS  => CUR_SAMPLE_POS_P,
				COEFF_LEFT  => LEFT_COEFFICIENTS( k_ProcessingUnit)(k_ProcessingUnit),
				COEFF_RIGHT => RIGHT_COEFFICIENTS(k_ProcessingUnit)(k_ProcessingUnit),
				SAMPLE      => SAMPLE_BUFFER(k_ProcessingUnit)(k_ProcessingUnit),
				CARRY_IN    => RESULT(k_ProcessingUnit-1),
				RESULT_OUT  => RESULT(k_ProcessingUnit),
				ENABLE_OUT  => PU_ENABLE_OUT(k_ProcessingUnit)
			);
	end generate;
	
	
	POS_COEFFICIENT_MEMORY_inst : RSMPL_COEFFICIENT_MEMORY
		port map (
			CLK         => CLK,
			SAMPLE_POS  => CUR_SAMPLE_POS_P(CUR_SAMPLE_POS_P'high downto CUR_SAMPLE_POS_P'length-C_TABLE_TIME_RESOLUTION_N_BITS),
			COEFF_LEFT  => LEFT_COEFFICIENTS( 0)(C_N_COEFFICIENTS/2 to C_N_COEFFICIENTS-1),
			COEFF_RIGHT => RIGHT_COEFFICIENTS(0)(C_N_COEFFICIENTS/2 to C_N_COEFFICIENTS-1),
		);
	
	
	NEG_COEFFICIENT_MEMORY_inst : RSMPL_COEFFICIENT_MEMORY
		port map (
			CLK         => CLK,
			SAMPLE_POS  => CUR_SAMPLE_POS_N(CUR_SAMPLE_POS_N'high downto CUR_SAMPLE_POS_N'length-C_TABLE_TIME_RESOLUTION_N_BITS),
			COEFF_LEFT  => LEFT_COEFFICIENTS( 0)(0 to C_N_COEFFICIENTS/2-1),
			COEFF_RIGHT => RIGHT_COEFFICIENTS(0)(0 to C_N_COEFFICIENTS/2-1),
		);
	
	
	p_DelayProcess : process (CLK)
	begin
		if rising_edge(CLK) then
			SAMPLE_BUFFER(     N_PIPELINE_STAGES-1 downto 1) <= SAMPLE_BUFFER(     N_PIPELINE_STAGES-2 downto 0);
			RIGHT_COEFFICIENTS(N_PIPELINE_STAGES-1 downto 1) <= RIGHT_COEFFICIENTS(N_PIPELINE_STAGES-2 downto 0);
			LEFT_COEFFICIENTS( N_PIPELINE_STAGES-1 downto 1) <= LEFT_COEFFICIENTS( N_PIPELINE_STAGES-2 downto 0);
		end if;
	end process;
	
	
end architecture;
