library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library RSMPL;
use RSMPL.RSMPL_PKG.all;


package RSMPL_COMP_PKG is
	
	
	component RSMPL_PROCESSING_UNIT is
		generic (
			USE_EARLY_EN : std_logic
		);
		port (
			CLK         : in  std_logic;
			EARLY_EN    : in  std_logic;
			LATE_EN     : in  std_logic;
			SAPMLE_POS  : in  unsigned (RSMPL_FACTOR_FRACTIONAL_N_BITS-1 downto 0);
			COEFF_LEFT  : in  signed (C_COEFFICIENTS_N_BITS-1 downto 0);
			COEFF_RIGHT : in  signed (C_COEFFICIENTS_N_BITS-1 downto 0);
			SAMPLE      : in  signed (ADC_N_BITS-1 downto 0);
			CARRY_IN    : in  signed (ADC_N_BITS-1 downto 0);
			RESULT_OUT  : out signed (ADC_N_BITS-1 downto 0);
			ENABLE_OUT  : out std_logic
		);
	end component;
	
	
end package;
