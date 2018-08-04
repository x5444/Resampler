package RSMPL_PKG is
	
	
	constant ADC_N_BITS                     : integer := 14;
	constant RSMPL_FACTOR_N_BITS            : integer := 32;
	constant RSMPL_FACTOR_FRACTIONAL_N_BITS : integer := 16;
	constant RSMPL_FACTOR_INTEGER_N_BITS    : integer := RSMPL_FACTOR_N_BITS - RSMPL_FACTOR_FRACTIONAL_N_BITS;
	
	constant C_STEP_WIDTH : integer := 2**RSMPL_FACTOR_FRACTIONAL_N_BITS;
	
	
end package;
