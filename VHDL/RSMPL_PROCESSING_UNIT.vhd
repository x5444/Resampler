library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library RSMPL;
use RSMPL.RSMPL_PKG.all;

entity RSMPL_PROCESSING_UNIT is
	generic (
		USE_EARLY_EN : std_logic
	);
	port (
		CLK         : in  std_logic;
		EARLY_EN    : in  std_logic;
		LATE_EN     : in  std_logic;
		SAPMLE_POS  : in  unsigned (RSMPL_FACTOR_FRACTIONAL_N_BITS-1 downto 0);
		COEFF_LEFT  : in  signed (C_COEFFICIENT_N_BITS-1 downto 0);
		COEFF_RIGHT : in  signed (C_COEFFICIENT_N_BITS-1 downto 0);
		SAMPLE      : in  signed (ADC_N_BITS-1 downto 0);
		CARRY_IN    : in  signed (ADC_N_BITS-1 downto 0);
		RESULT_OUT  : out signed (ADC_N_BITS-1 downto 0);
		ENABLE_OUT  : out std_logic
	);
end entity;
	
	
architecture RTL of RSMPL_PROCESSING_UNIT is
begin
	
	
	p_mainCalc : process (CLK)
	begin
		if rising_edge(CLK) then
			
			-- This converts between the time interpolation and amplitude
			-- interpolation. We only need to check those bits from the sample position,
			-- which aren't used by the table lookup. At the same time, we can ignore
			-- those bits, which won't influence the result b/c time interpolation is
			-- more precise than amplitude interpolation.
			--
			vFracMetric := SAMPLE_POS (SAMPLE_POS'high - C_TABLE_TIME_RESOLUTION_N_BITS downto
			                           C_TIME_INTERPOLATION_FACTOR_N_BITS - C_AMPL_INTERPOLATION_FACTOR_N_BITS);
			FRAC_RIGHT  <=                                           vFracMetric;
			FRAC_LEFT   <= (2**C_AMPL_INTERPOLATION_FACTOR_N_BITS) - vFracMetric;
			
			COEFF <= FRAC_RIGHT * COEFF_RIGHT + FRAC_LEFT * COEFF_LEFT;
			
			RESULT_OUT <= CARRY_IN + SAMPLE * COEFF;
			
		end if;
	end process;
	
	
end architecture;
