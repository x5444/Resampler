library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library RSMPL;
use RSMPL.RSMPL_PKG.all;


entity RSMPL_TOP is
	port (
		CLK : in  std_logic;
		
		DATA_IN : in  std_logic_vector (ADC_N_BITS-1 downto 0);
		
		DATA_OUT    : out std_logic_vector (ADC_N_BITS-1 downto 0);
		DATA_OUT_EN : out std_logic;
		
		CONF_RSMPL_FACTOR : in  unsigned (RSMPL_FACTOR_N_BITS-1 downto 0)
	);
end entity;


architecture RTL of RSMPL_TOP is
	
	type SAMPLE_BUFFER_t is array (0 to 5) of std_logic_vector (ADC_N_BITS-1 downto 0);
	signal SAMPLE_BUFFER : SAMPLE_BUFFER_t;
	
	signal NEXT_SAMPLE_POS : unsigned (RSMPL_FACTOR_N_BITS-1            downto 0);
	signal  CUR_SAMPLE_POS : unsigned (RSMPL_FACTOR_FRACTIONAL_N_BITS-1 downto 0);
	
	signal ENABLE_OUTPUT : std_logic;
	
begin
	
	
	-- Move input data into the sample buffer.
	--
	p_sampleBuffer : process (CLK)
	begin
		if rising_edge(CLK) then
			SAMPLE_BUFFER(1 to SAMPLE_BUFFER'high) <= SAMPLE_BUFFER(0 to SAMPLE_BUFFER'high-1);
			SAMPLE_BUFFER(0)                           <= DATA_IN;
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
				CUR_SAMPLE_POS  <= (others => '0');
				NEXT_SAMPLE_POS <= NEXT_SAMPLE_POS - C_STEP_WIDTH;
				ENABLE_OUTPUT   <= '0';
				
			-- If it's less than a step width, we need to start doing some
			-- calculations.
			--
			else
				assert NEXT_SAMPLE_POS < 2**CUR_SAMPLE_POS'high report "Error: out of range" severity FAILURE;
				CUR_SAMPLE_POS  <= NEXT_SAMPLE_POS (CUR_SAMPLE_POS'high downto 0);
				NEXT_SAMPLE_POS <= CUR_SAMPLE_POS + CONF_RSMPL_FACTOR - C_STEP_WIDTH;
				ENABLE_OUTPUT   <= '1';
			end if;
		end if;
	end process;
	
	
	LEFT_COEFFICIENT_MEMORY_inst : RSMPL_COEFFICIENT_MEMORY
		port map (
			CLK        => CLK,
			SAMPLE_POS => CUR_SAMPLE_POS,
			COEFF      => LEFT_COEFFICIENTS
		);
	
	RIGHT_COEFFICIENT_MEMORY_inst : RSMPL_COEFFICIENT_MEMORY
		port map (
			CLK        => CLK,
			SAMPLE_POS => CUR_SAMPLE_POS + to_unsigned(1, CUR_SAMPLE_POS'length),
			COEFF      => RIGHT_COEFFICIENTS
		);
	
	
end architecture;
