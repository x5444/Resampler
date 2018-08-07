package RSMPL_PKG is
	
	
	constant ADC_N_BITS                     : integer := 14;
	constant RSMPL_FACTOR_N_BITS            : integer := 32;
	constant RSMPL_FACTOR_FRACTIONAL_N_BITS : integer := 16;
	constant RSMPL_FACTOR_INTEGER_N_BITS    : integer := RSMPL_FACTOR_N_BITS - RSMPL_FACTOR_FRACTIONAL_N_BITS;
	
	constant C_STEP_WIDTH              : integer := 2**RSMPL_FACTOR_FRACTIONAL_N_BITS;
	constant C_TABLE_AMPL_RESOLUTION_N_BITS : integer := 12;
	constant C_TABLE_TIME_RESOLUTION_N_BITS : integer := 10;
	
	constant C_N_COEFFICIENTS : integer := 6;
	constant C_COEFFICIENT_N_BITS : integer := 16;
	
	
end package;
