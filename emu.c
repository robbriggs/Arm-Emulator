#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/**** Data type definitions ****/

typedef enum {
	MOV,
	ADD,
	SUB,
	MUL,
	MLA,
	CMP,
	AND,
	EOR,
	ORR,
	BIC,
	B,
	BL,
	LDR,
	STR,
	SVC,
	PUSH,
	POP
} InstructionName;

typedef struct {
	int N;
	int Z;
	int C;
	int V;
	int Vaffected;
} Condition;

typedef enum {
	LSL,
	LSR,
	None
} ShiftOperator;

typedef enum { Reg, Imm } OperandType;

typedef struct {
	OperandType type;
	int val;
} Operand;

typedef struct {
	Operand value;
	Operand value2;
	Operand value3;
	Operand shift;
	ShiftOperator operator;
	int U;
	int P;
	int W;
} ShifterMeaning;

typedef struct {
	int val;
	int C;
} ShifterResult;

typedef struct {
	InstructionName name;
	Condition* con;
	int S;
	Operand op1;
	Operand op2;
	Operand op3;
	Operand op4;
	ShifterMeaning shift;
} Instruction;

typedef struct {
	int value;
	Condition cond;
} ResultAnalysis;

typedef struct l {
	int value;
	struct l *next;
} List;

typedef struct s {
	struct s* next;
	struct s* prev;
	int address;
	char data[9];
	int empty;
	int changed;
} Source;

typedef struct {
	int trace;
	int before;
	int after;
} Flags;

typedef struct {
	List* stack;
	int* reg;
	Condition CPRS;
	Source* memory;
	Flags flags;
} SystemValues;



/*** Subroutines used throught the program ***/

void killProgram( char* msg )
{
	printf("Error in program:\n%s\n", msg );
	exit(0);
}

/*** String Manipulation ***/

char sBit( char* code, int bit )
{
	return code[31-bit];
}

void addStrings( char** dest, char* new )
{
	char* output = malloc( ( strlen( *dest ) + strlen( new ) + 1 ) * sizeof( char ) );
	strcpy( output, *dest );
	strcat( output, new );
	/*free( *dest );*/
	*dest = output;
}

char* addDigit( int* c, char* working, int digit )
{
	char* newChar;
	char* output = malloc( strlen( working ) + 3 );

	if ( *c >= digit )
	{
		*c -= digit;
		newChar = "1";
	}
	else {newChar = "0";}

	strcpy( output, working );
	strcat( output, newChar );

	return output;
}

char* extractSubString( char* str, int start, int end )
{
	char* output = malloc( ( end - start + 1 ) * sizeof( char ) );
	int i;

	for ( i = start; i <= end; i++ )
	{
		output[i - start] = str[i];
	}

	output[end - start + 1] = '\0';
	return output;
}

/*** Numberical Manipulation ***/

int ipow(int base, int exp)
{
    int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

int max( int num1, int num2 )
{
	if ( num1 > num2 )
		return num1;
	else
		return num2;
}

int numberSign( int num )
{
	return ( num < 0 ) ? -1 : 1;
}

int convertHexChar( char c )
{
	if ( isdigit( c ) )
		return c - 48;
	else if ( c < 71 && c > 64 )
		return c - 55;
	else if ( c < 103 && c > 96 )
		return c - 87;
	else
		killProgram( "Invalid character inputted into convertToHexChar()" );
		
	return 1;
}

int extractBin( char** ptr )
{
	int result = 1;

	if ( **ptr == '1' )
		result = 1;
	else if ( **ptr == '0' )
		result = 0;
	else
		killProgram( "Invalid char in extractBin" );


	(*ptr)++;

	return result;
}

char* convertToBin( char* hex )
{
	char* working = "";
	int c;

	while ( *hex != '\0' )
	{
		c = convertHexChar( *hex );

		working = addDigit( &c, working, 8 );
		working = addDigit( &c, working, 4 );
		working = addDigit( &c, working, 2 );
		working = addDigit( &c, working, 1 );

		hex++;
	}

	return working;
}

int binToDec( char* bin )
{
	int dec = 0;

	while ( *bin != '\0' )
	{
		dec = dec * 2 + (*bin - 48);
		bin++;
	}

	return dec;
}

char* intToString( int num )
{
	char* buffer = malloc( 6 * sizeof( char ) );
	sprintf( buffer, "%d", num );
	return buffer;
}

int HexToDec( char* hex )
{
	int bin = 0;

	while ( *hex != '\0' )
	{
		bin = ( bin * 16 ) + convertHexChar( *hex );
		hex++;
	}

	return bin;
}

char flipHexChar( char hex )
{
	switch ( hex )
	{
		case '0':
			return 'F';
			break;
		case '1':
			return 'E';
			break;
		case '2':
			return 'D';
			break;
		case '3':
			return 'C';
			break;
		case '4':
			return 'B';
			break;
		case '5':
			return 'A';
			break;
		case '6':
			return '9';
			break;
		case '7':
			return '8';
			break;
		case '8':
			return '7';
			break;
		case '9':
			return '6';
			break;
		case 'A':
			return '5';
			break;
		case 'B':
			return '4';
			break;
		case 'C':
			return '3';
			break;
		case 'D':
			return '2';
			break;
		case 'E':
			return '1';
			break;
		case 'F':
			return '0';
			break;
		
	}
	
	killProgram("NegDecToHex, invalide letter");
}

char* DecToHex( int dec );

char* NegDecToHex( int dec )
{
	dec += 1;
	char* output = DecToHex( dec * -1 );
	
	int i;
	for ( i = 0; i < 8; i++ )
		output[i] = flipHexChar( output[i] );
		
	return output;
}

char* DecToHex( int dec )
{
	int i;
	int digVal;
	
	if ( dec < 0 )
		return NegDecToHex( dec );
	
	char* output = malloc( 9 * sizeof( char ) );
	output[8] = '\0';
	
	for ( i = 0; i < 8; i++ )
	{
		digVal = dec % 16;
		dec = dec >> 4;
		
		if ( digVal > 9 )
			output[7-i] = digVal + 55;
		else
			output[7-i] = digVal + 48;
	}
	
	return output;
}

char* UnDecToHex( unsigned int dec )
{
	int i;
	int digVal;
	
	char* output = malloc( 9 * sizeof( char ) );
	output[8] = '\0';
	
	for ( i = 0; i < 8; i++ )
	{
		digVal = dec % 16;
		dec = dec >> 4;
		
		if ( digVal > 9 )
			output[7-i] = digVal + 55;
		else
			output[7-i] = digVal + 48;
	}
	
	return output;
}

int unTC( int comp, int digits )
{
    int threshold = ipow( 2, digits - 1 ) - 1;
    int max = ipow( 2, digits ) - 1;

    if ( comp > threshold )
    {
        return comp - max - 1;
    }

    else
        return comp;

}

/*** Memory/Register Interaction ***/

Source* getMemoryLocation( int index, Source* source )
{
	if ( source -> address < index )
	{
		do { source = source -> next; }
		while ( source -> address != index );
	}
	else if ( source -> address > index )
	{
		do { source = source -> prev;}
		while ( source -> address != index);
	}
	
	return source;
}

int getReg( int index, SystemValues* vals )
{
	/*printf("Returning reg %i as %i\n", index, vals -> reg[index] );*/
	return vals -> reg[index];
}

Source* updatePos( Source* source, SystemValues* vals )
{
	return getMemoryLocation( getReg( 15, vals ), source );
}

void setReg( int index, int value, SystemValues* vals )
{
	vals -> reg[index] = value;
}

void print8digit( int num )
{
	int i;
	int output[8];
	
	if ( num > -1 )
		printf(" ");
	else
	{
		printf("-");
		num *= -1;
	}

	for ( i = 7; i >= 0; i-- )
	{
		output[i] = num % 10;

		num = (num-output[i]) / 10;
	}

	for ( i = 0; i < 8; i++ )
	{
		printf("%i", output[i]);
	}
}

void printRegisters( SystemValues* vals )
{
	int i;
	for ( i = 0; i < 13; i++ )
	{
		if( i == 0 )
			printf( "R%i  = ", i );
		else if ( i < 10 )
			printf( "R%i  = ", i );
		else
			printf( "R%i = ", i );
		print8digit( getReg( i, vals ) );

		if ( (i+1) % 4 )
			printf(", ");
		else
			printf("\n");
	}
	printf( "SP  = " );
	print8digit( getReg( 13, vals ) );
	printf( ", LR  = " );
	print8digit( getReg( 14, vals ) );
	printf( ", PC  = " );
	print8digit( getReg( 15, vals ) );
	printf("\n");
}

void printCPRS( SystemValues* vals )
{
	printf( "N = %i, Z = %i, C = %i, V = %i\n", vals->CPRS.N, vals->CPRS.Z, vals->CPRS.C, vals->CPRS.V);
}

/*** Execute Instruction from datatype ***/

int conditionsMet( Condition* con, SystemValues* vals )
{
	Condition flags = vals -> CPRS;

	if ( con -> N == 1 )
	{
		if ( con -> Z == 1 )
		{
			if ( con -> C == 1 )
			{
				if ( con -> V == 0 )
				{
					return 1;
				}
			}else{
				if ( con -> V == 1 )
				{
					/* LE */
					return ( flags.Z == 1 ||
					         flags.N != flags.V );
				}else{
					/* GT */
					return ( flags.Z == 1 &&
							 flags.N == flags.V );
				}
			}
		}else{
			if ( con -> C == 1 )
			{
				if ( con -> V == 1 )
				{
					/* LT */
					return ( flags.N != flags.V );
				}else{
					/* GE */
					return ( flags.N == flags.V );
				}
			}
		}
	}else{
		if ( con -> Z == 1 )
		{
			if ( con -> C == 0 )
			{
				if ( con -> V == 1 )
				{
					/* PL */
					return !( flags.V );
				}else{
					/* MI */
					return ( flags.V );
				}
			}
		}else{
			if ( con -> C == 0 )
			{
				if ( con -> V == 1 )
				{
					/* NE */
					return !( flags.Z );
				}else{
					/* EQ */
					return ( flags.Z );
				}
			}
		}
	}

	killProgram( "Condition unknown" );
	return 1;
}


Condition startCondition( int result )
{
	Condition output;
	output.N = ( result < 0 );
	output.Z = ( result == 0 );
	output.V = 0;
	output.Vaffected = 1;
	return output;
}

int operandValue( Operand op, SystemValues* vals )
{
	if ( op.type == Reg )
		return getReg( op.val, vals );
	else
		return op.val;
}

int calcShift( ShifterMeaning shift, SystemValues* vals )
{
	if ( shift.operator == None )
		return operandValue(shift.value, vals);

	if ( shift.operator == LSL )
		return operandValue( shift.value, vals ) << operandValue( shift.shift, vals );
	else
		return shift.value.val >> operandValue( shift.shift, vals );
}

void setCV( int a, int b, int c, Condition* con )
{
	if ( ( b > -1 ) != ( c > -1 ) )
	{
		/* Different signs */
		con -> V = 0; /* Can't overflow */
		con -> C = ( a > -1 );
	}
	else if ( b > -1 )
	{
		/* Both + */
		con -> V = ( a < 0 );
		con -> C = 0;
	}else{
		/* Both - */
		con -> V = 0; /* Can't overflow */
		con -> C = ( a > -1 );
	}
}

Condition executeADD( Instruction* inst, SystemValues* vals )
{
	/* a = b + c */
	Condition output;
	int b = operandValue( inst -> op2, vals );
	int c = calcShift( inst -> shift, vals );
	int a = b + c;
	setReg( (inst -> op1).val, a, vals );

	output = startCondition( a );
	setCV( a, b, c, &output );
	
			
	

	return output;
}

Condition executeSUB( Instruction* inst, SystemValues* vals )
{
	/* a = b - c */
	Condition output;
	int b = operandValue( inst -> op2, vals );
	int c = calcShift( inst -> shift, vals );
	int a = b - c;
	setReg( (inst -> op1).val, a, vals );

	output = startCondition( a );
	setCV( a, b, -c, &output );
	return output;
}

Condition executeMOV( Instruction* inst, SystemValues* vals )
{
	/* a = b */
	Condition output;
	int b = calcShift( inst -> shift, vals );

	setReg( (inst -> op1).val, b, vals );

	output = startCondition( b );
	output.Vaffected = 0;
	return output;
}

Condition executeMUL( Instruction* inst, SystemValues* vals )
{
	/* a = b * c */
	Condition output;
	int b = operandValue( inst -> op2, vals );
	int c = operandValue( inst -> op3, vals );
	int a = b * c;
	setReg( (inst -> op1).val, a, vals );

	output = startCondition( a );
	output.Vaffected = 0;

	return output;
}

Condition executeMLA( Instruction* inst, SystemValues* vals )
{
	/* a = (b * c) + d */
	Condition output;
	int b = operandValue( inst -> op2, vals );
	int c = operandValue( inst -> op3, vals );
	int d = operandValue( inst -> op4, vals );
	int a = (b * c) + d;
	setReg( (inst -> op1).val, a, vals );

	output = startCondition( a );
	output.Vaffected = 0;
	
	return output;
}

Condition executeAND( Instruction* inst, SystemValues* vals )
{
	/* a = b & c */
	Condition output;
	int b = operandValue( inst -> op2, vals );
	int c = calcShift( inst -> shift, vals );
	int a = b & c;
	setReg( (inst -> op1).val, a, vals );

	output = startCondition( a );
	output.Vaffected = 0;

	return output;
}

void executeBL( SystemValues* vals )
{
    int returnAddress = getReg( 15, vals );
    setReg( 14, returnAddress, vals );
}

void executeB( Instruction* inst, SystemValues* vals )
{
    int address = inst -> op1.val + getReg(15, vals) + 4;
    setReg( 15, address, vals );
}

Condition executeORR( Instruction* inst, SystemValues* vals )
{
	/* a = b | c */
	Condition output;
	int b = operandValue( inst -> op2, vals );
	int c = calcShift( inst -> shift, vals );
	int a = b | c;
	setReg( (inst -> op1).val, a, vals );

	output = startCondition( a );
	output.Vaffected = 0;

	return output;
}

Condition executeEOR( Instruction* inst, SystemValues* vals )
{
	/* a = b ^ c */
 	Condition output;
	int b = operandValue( inst -> op2, vals );
	int c = calcShift( inst -> shift, vals );
	int a = b ^ c;
	setReg( (inst -> op1).val, a, vals );

	output = startCondition( a );
	output.Vaffected = 0;

	return output;
}

Condition executeBIC( Instruction* inst, SystemValues* vals )
{
	/* a = b & (~c) */
	Condition output;
	int b = operandValue( inst -> op2, vals );
	int c = calcShift( inst -> shift, vals );
	int a = b & (~c);
	setReg( (inst -> op1).val, a, vals );

	output = startCondition( a );
	output.Vaffected = 0;

	return output;
}

Condition executeCMP( Instruction* inst, SystemValues* vals )
{
	/* a = b - c -> same as SUB but the result isnt stored */
	Condition output;
	int b = operandValue( inst -> op1, vals );
	int c = calcShift( inst -> shift, vals );
	int a = b - c;

	output = startCondition( a );
	setCV( a, b, -c, &output );
	return output;
}

int calcAddress( Instruction* inst, SystemValues* vals )
{
	/* index = PC + offset */
	int offset = calcShift( inst -> shift, vals );
	if ( inst -> shift.U )
		return operandValue( inst -> op2, vals ) + offset +4;
	else
		return operandValue( inst -> op2, vals ) - offset +4;
}

void executeLDR( Instruction* inst, SystemValues* vals )
{
	/* a = memory[index] */
	int index = calcAddress( inst, vals );
	Source* memory = getMemoryLocation( index, vals -> memory );
	char hex = (memory -> data)[0];
	char data[9];
	data[8] = '\0';
	
	int i;
	for ( i = 0; i < 8; i++ )
	{
		data[i] = (memory -> data)[i];
	}

	setReg( (inst -> op1).val, unTC( HexToDec(data), 32 ), vals ); 
	
	if ( inst -> shift.W )
		setReg( (inst -> op2).val, index, vals );
}

void executeSTR( Instruction* inst, SystemValues* vals )
{
	/* memory[index] = a */
	int index = calcAddress( inst, vals );
	Source* memory = getMemoryLocation( index, vals -> memory );
	int a = getReg( inst->op1.val, vals );
	char* achar = DecToHex( a );
	int i;
	for ( i = 0; i < 8; i++ )
	{
		memory -> data[i] = achar[i];
	}
	
	if ( inst -> shift.W )
		setReg( (inst -> op2).val, index, vals );
		
	memory -> changed = 1;
}

void updateCPRS( Condition con, SystemValues* vals )
{
	vals -> CPRS.N = con.N;
	vals -> CPRS.Z = con.Z;
	vals -> CPRS.C = con.C;
	
	if ( vals -> CPRS.Vaffected )
		vals -> CPRS.V = con.V;
}

int executeInstuction( Instruction* inst, SystemValues* vals )
{
	Condition con;
	switch ( inst -> name )
	{
		case ADD:
			con = executeADD( inst, vals );
			break;
		case SUB:
			con = executeSUB( inst, vals );
			break;
		case MOV:
			con = executeMOV( inst, vals );
			break;
		case MUL:
			con = executeMUL( inst, vals );
			break;
		case MLA:
			con = executeMLA( inst, vals );
			break;
		case AND:
			con = executeAND( inst, vals );
			break;
		case ORR:
			con = executeORR( inst, vals );
			break;
		case EOR:
			con = executeEOR( inst, vals );
			break;
		case BIC:
			con = executeBIC( inst, vals );
			break;
		case CMP:
			con = executeCMP( inst, vals );
			break;
		case SVC:
			if ( inst -> op1.val == 0 )
				return 1;
			else if ( inst -> op1.val == 1 )
				printRegisters( vals );
			else if ( inst -> op1.val == 2)
				printf("r0 = %s\n", DecToHex( getReg( 0, vals ) ) );
			else
				killProgram("SVI error"); 
			break;
        case BL:
            executeBL( vals );
        case B:
            executeB( inst, vals );
            break;
        case LDR:
        	executeLDR( inst, vals );
        	break;
        case STR:
        	executeSTR( inst, vals );
        	break;
        default:
            killProgram("Instruction error");
            break;
	}

	if ( inst -> name == CMP ||
	     ( inst -> S &&
	     !( inst -> name == B ||
	        inst -> name == BL ||
	        inst -> name == LDR ||
	        inst -> name == STR ) ) )
	     
	     updateCPRS( con, vals );

	return 0;
}

/*** Print Instruction from datatype ***/

char* extractInstName( Instruction* inst )
{
	switch ( inst -> name )
	{
		case MOV:
			return "MOV";
		case ADD:
			return "ADD";
		case SUB:
			return "SUB";
		case MUL:
			return "MUL";
		case MLA:
			return "MLA";
		case CMP:
			return "CMP";
		case AND:
			return "AND";
		case EOR:
			return "EOR";
		case ORR:
			return "ORR";
		case BIC:
			return "BIC";
		case B:
			return "B";
		case BL:
			return "BL";
		case LDR:
			return "LDR";
		case STR:
			return "STR";
		case SVC:
			return "SVC";
		case PUSH:
			return "PUSH";
		case POP:
			return "POP";
		default:
			return "ERROR";
	}
	return "";
}

char* shiftOperatorToString( ShiftOperator shift )
{
	if ( shift == None )
		return ""; 
	else if ( shift == LSL )
		return "LSL";
	else if ( shift == LSR )
		return "LSR";
	else
		killProgram("Error in shiftOperatorToString");
	return "";
}

char* conditionString( Condition* con )
{
	if ( con -> N == 1 )
	{
		if ( con -> Z == 1 )
		{
			if ( con -> C == 1 )
			{
				if ( con -> V == 0 )
				{
					return "";
				}
			}else{
				if ( con -> V == 1 )
				{
					return "LE";
				}else{
					return "GT";
				}
			}
		}else{
			if ( con -> C == 1 )
			{
				if ( con -> V == 1 )
				{
					return "LT";
				}else{
					return "GE";
				}
			}
		}
	}else{
		if ( con -> Z == 1 )
		{
			if ( con -> C == 0 )
			{
				if ( con -> V == 1 )
				{
					return "PL";
				}else{
					return "MI";
				}
			}
		}else{
			if ( con -> C == 0 )
			{
				if ( con -> V == 1 )
				{
					return "NE";
				}else{
					return "EQ";
				}
			}
		}
	}

	killProgram( "Condition unknown" );
	return "";
}

char* operandToString( Operand op )
{
	char* output = "";

	if ( op.type == Reg )
	{
		if ( op.val == 13 )
			return "sp";
		else if ( op.val == 14)
			return "lr";
		else if ( op.val == 15)
			return "pc";
		else
			addStrings( &output, "r" );
	}
	else
		addStrings( &output, "#" );

	addStrings( &output, intToString( op.val ) );

	return output;
}

char* shifterToString( ShifterMeaning shift )
{
	char* output = operandToString( shift.value );

	if ( shift.shift.val != 0 && shift.operator != None )
	{
		addStrings( &output, " " );
		addStrings( &output, shiftOperatorToString( shift.operator ) );
		addStrings( &output, " " );
		addStrings( &output, operandToString( shift.shift ) );
	}
	return output;
}

char* addressInstToString( Instruction* inst )
{
	char* output = extractInstName( inst );
	addStrings( &output, conditionString( inst -> con ) );
	
	addStrings( &output, " " );
	addStrings( &output, operandToString( inst -> op1 ) );
	addStrings( &output, ", [" );
	addStrings( &output, operandToString( inst -> op2 ) );
	
	if ( inst -> shift.P)
	{
		addStrings( &output, ", " );
			
		if ( inst -> shift.U )
			addStrings( &output, "+" );
		else
			addStrings( &output, "-" );
			
		addStrings( &output, operandToString( inst -> shift.value ) );
	}
	
	addStrings( &output, "]" );
	
	if ( inst -> shift.W )
		addStrings( &output, "!" );
		
	if ( !inst -> shift.P )
	{
		addStrings( &output, ", " );
	
		if ( inst -> shift.U )
			addStrings( &output, "+" );
		else
			addStrings( &output, "-" );
		
		addStrings( &output, operandToString( inst -> shift.value ) );
	}
	
	return output;
}

char* instructionToString( Instruction* inst )
{
	char* output = extractInstName( inst );

	if ( inst -> name == LDR || inst -> name == STR )
		return addressInstToString( inst );

	addStrings( &output, conditionString( inst -> con ) );

	if ( inst -> S && inst -> name != CMP )
		addStrings( &output, "S" );

	addStrings( &output, " " );
	addStrings( &output, operandToString( inst -> op1 ) );

	if ( inst -> name == SVC || inst -> name == B || inst -> name == BL )
		return output;

	addStrings( &output, ", " );

	if ( inst -> name != MOV && inst -> name != CMP )
	{
		addStrings( &output, operandToString( inst -> op2 ) );
		addStrings( &output, ", " );
	}

	if ( inst -> name == MUL || inst -> name == MLA )
		addStrings( &output, operandToString( inst -> op3 ) );
	else
		addStrings( &output, shifterToString( inst -> shift ) );

	if ( inst -> name == MLA )
	{
		addStrings( &output, ", " );
		addStrings( &output, operandToString( inst -> op4 ) );
	}
	return output;

}

void printSource( Source* curPos )
{
	char* adr;
	while ( curPos != NULL && curPos -> empty == 0 )
	{
		adr = DecToHex( curPos -> address );
		printf("%s %s\n", adr, curPos -> data);
		free( adr ); 
		curPos = curPos -> next;
	}
}

void printChangedSource( Source* curPos )
{
	char* adr;
	while ( curPos != NULL && curPos -> empty == 0  )
	{
		if ( curPos -> changed == 1 )
		{
			adr = DecToHex( curPos -> address );
			printf("%s %s\n", adr, curPos -> data);
			free( adr );
			
		}
		curPos = curPos -> next;
	}
}


/*** Make Instruction from Hex ***/


Condition* makeCondition( char* in )
{
	Condition* con = (Condition *) malloc( sizeof( Condition ) );
	char** ptr = &in;
	con -> N = extractBin( ptr );
	con -> Z = extractBin( ptr );
	con -> C = extractBin( ptr );
	con -> V = extractBin( ptr );

	return con;
}

int whichIndex( char* code )
{
	if ( sBit( code, 24 ) == '0' ) /* P bit */
		return 2;
	else if ( sBit( code, 21 ) == '0' ) /* W bit */
		return 0;
	
	return 1;
}

ShifterMeaning dataProcessingShifter( char* code )
{
	ShifterMeaning output;
	int roate_imm;
	int immed_8;
	int shift;
	int Rs;
	int Rm;
	if ( sBit( code, 25 ) == '1' ) /* I */
	{
		/* 32-bit Immediate */
		roate_imm = binToDec( extractSubString( code, 31-11, 31-8 ) );
		immed_8 = binToDec( extractSubString( code, 31-7, 31-0 ) );
		output.operator = None;
		output.value.type = Imm;
		output.value.val = immed_8;
	}else{
		if ( sBit( code, 4 ) == '0' )
		{
			/* Immediate shift */
			output.shift.val = binToDec( extractSubString( code, 31-11, 31-7 ) );
			output.value.val = binToDec( extractSubString( code, 31-3, 31-0 ) );
			output.shift.type = Imm;
			output.value.type = Reg;
			shift = binToDec( extractSubString( code, 31-6, 31-5 ) );

			if ( shift == 0 )
				output.operator = LSL;
			else if ( shift == 1 )
				output.operator = LSR;
			else
				killProgram("Instruction has something more complicated than LSL or LSR");

		}else{
			/* Register shift */
			Rs = binToDec( extractSubString( code, 31-11, 31-8 ) );
			Rm = binToDec( extractSubString( code, 31-3, 31-0 ) );
			output.shift.type = Reg;
			output.value.type = Reg;
			shift = binToDec( extractSubString( code, 31-7, 31-5 ) );

			if ( shift == 0 )
				output.operator = LSL;
			else if ( shift == 1 )
				output.operator = LSR;
			else
				killProgram("Instruction has something more complicated than LSL or LSR");
		}
	}

	return output;
}

ShifterMeaning addressValue( char* code )
{
	ShifterMeaning output;

	if ( sBit( code, 25 ) == '0' )
	{
		/* Immemmediate offset */
		output.operator = None;
		output.value.val = binToDec( extractSubString( code, 31-11, 31-0 ) );
		output.value.type = Imm;
	}else{
		/* Register */
		int shift_imm = binToDec( extractSubString( code, 31-11, 31-7 ) );
		if ( shift_imm == 0 )
		{
			output.operator = None;
		}else if ( sBit( code, 6 ) == '0' &&
		           sBit( code, 5 ) == '0' )
		{
			output.operator = LSL;
			output.shift.type = Imm;
			output.shift.val = binToDec( extractSubString( code, 31-11, 31-7 ) );
		}else if ( sBit( code, 6 ) == '0' &&
		           sBit( code, 5 ) == '1' )
		{
			output.operator = LSR;
			output.shift.type = Imm;
			output.shift.val = binToDec( extractSubString( code, 31-11, 31-7 ) );
		}else
			killProgram("Instruction has something more complicated than LSL or LSR");
			
		output.value.val = binToDec( extractSubString( code, 31-3, 31-0 ) );
		output.value.type = Reg;
	}
	output.P = sBit( code, 24 ) - 48;
	output.U = sBit( code, 23 ) - 48;
	output.W = sBit( code, 21 ) - 48;
	
	
	return output;
}

Instruction* makeInstruction( char* hex )
{
	char* code = convertToBin( hex );

	Instruction* inst = malloc( sizeof( Instruction ) );

	inst -> con = makeCondition( code );
	inst -> S = sBit( code, 20 ) - 48;

	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 24 ) == '0' &&
	     sBit( code, 23 ) == '1' &&
	     sBit( code, 22 ) == '0' &&
	     sBit( code, 21 ) == '0' )
	{
		/* ADD */
		inst -> name = ADD;
		inst -> op1.val = binToDec( extractSubString( code, 31-15, 31-12 ) );
		inst -> op2.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op1.type = Reg;
		inst -> op2.type = Reg;
		inst -> shift = dataProcessingShifter( code );
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 24 ) == '0' &&
	     sBit( code, 23 ) == '0' &&
	     sBit( code, 22 ) == '1' &&
	     sBit( code, 21 ) == '0' )
	{
		/* SUB */
		inst -> name = SUB;
		inst -> op1.val = binToDec( extractSubString( code, 31-15, 31-12 ) );
		inst -> op2.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op1.type = Reg;
		inst -> op2.type = Reg;
		inst -> shift = dataProcessingShifter( code );
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 24 ) == '0' &&
	     sBit( code, 23 ) == '0' &&
	     sBit( code, 22 ) == '0' &&
	     sBit( code, 21 ) == '0' &&
	     sBit( code, 7  ) == '1' &&
	     sBit( code, 6  ) == '0' &&
	     sBit( code, 5  ) == '0' &&
	     sBit( code, 4  ) == '1' )
	{
		/* MUL */
		inst -> name = MUL;
		inst -> op1.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op2.val = binToDec( extractSubString( code, 31-3, 31-0 ) );
		inst -> op3.val = binToDec( extractSubString( code, 31-11, 31-8 ) );
		inst -> op1.type = Reg;
		inst -> op2.type = Reg;
		inst -> op3.type = Reg;
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 25 ) == '0' &&
	     sBit( code, 24 ) == '0' &&
	     sBit( code, 23 ) == '0' &&
	     sBit( code, 22 ) == '0' &&
	     sBit( code, 21 ) == '1' &&
	     sBit( code, 7  ) == '1' &&
	     sBit( code, 6  ) == '0' &&
	     sBit( code, 5  ) == '0' &&
	     sBit( code, 4  ) == '1' )
	{
		/* MLA */
		inst -> name = MLA;
		inst -> op1.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op2.val = binToDec( extractSubString( code, 31-3, 31-0 ) );
		inst -> op3.val = binToDec( extractSubString( code, 31-11, 31-8 ) );
		inst -> op4.val = binToDec( extractSubString( code, 31-15, 31-12 ) );
		inst -> op1.type = Reg;
		inst -> op2.type = Reg;
		inst -> op3.type = Reg;
		inst -> op4.type = Reg;
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 24 ) == '0' &&
	     sBit( code, 23 ) == '0' &&
	     sBit( code, 22 ) == '0' &&
	     sBit( code, 21 ) == '0' )
	{
		/* AND */
		inst -> name = AND;
		inst -> op1.val = binToDec( extractSubString( code, 31-15, 31-12 ) );
		inst -> op2.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op1.type = Reg;
		inst -> op2.type = Reg;
		inst -> shift = dataProcessingShifter( code );
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 24 ) == '1' &&
	     sBit( code, 23 ) == '1' &&
	     sBit( code, 22 ) == '1' &&
	     sBit( code, 21 ) == '0' )
	{
		/* BIC */
		inst -> name = BIC;
		inst -> op1.val = binToDec( extractSubString( code, 31-15, 31-12 ) );
		inst -> op2.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op1.type = Reg;
		inst -> op2.type = Reg;
		inst -> shift = dataProcessingShifter( code );
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 24 ) == '1' &&
	     sBit( code, 23 ) == '0' &&
	     sBit( code, 22 ) == '1' &&
	     sBit( code, 21 ) == '0' )
	{
		/* CMP */
		inst -> name = CMP;
		inst -> op1.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op1.type = Reg;
		inst -> shift = dataProcessingShifter( code );
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 24 ) == '0' &&
	     sBit( code, 23 ) == '0' &&
	     sBit( code, 22 ) == '0' &&
	     sBit( code, 21 ) == '1' )
	{
		/* EOR */
		inst -> name = EOR;
		inst -> op1.val = binToDec( extractSubString( code, 31-15, 31-12 ) );
		inst -> op2.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op1.type = Reg;
		inst -> op2.type = Reg;
		inst -> shift = dataProcessingShifter( code );
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 24 ) == '1' &&
	     sBit( code, 23 ) == '1' &&
	     sBit( code, 22 ) == '0' &&
	     sBit( code, 21 ) == '0' )
	{
		/* ORR */
		inst -> name = ORR;
		inst -> op1.val = binToDec( extractSubString( code, 31-15, 31-12 ) );
		inst -> op2.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op1.type = Reg;
		inst -> op2.type = Reg;
		inst -> shift = dataProcessingShifter( code );
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 24 ) == '1' &&
	     sBit( code, 23 ) == '1' &&
	     sBit( code, 22 ) == '0' &&
	     sBit( code, 21 ) == '1' )
	{
		/* MOV */
		inst -> name = MOV;
		inst -> op1.val = binToDec( extractSubString( code, 31-15, 31-12 ) );
		inst -> op1.type = Reg;
		inst -> shift = dataProcessingShifter( code );
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '1' &&
	     sBit( code, 22 ) == '0' &&
	     sBit( code, 20 ) == '1' )
	{
		/* LDR */
		inst -> name = LDR;
		inst -> op1.val = binToDec( extractSubString( code, 31-15, 31-12 ) );
		inst -> op2.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op1.type = Reg;
		inst -> op2.type = Reg;
		inst -> shift = addressValue( code );
	}
	else
	if ( sBit( code, 27 ) == '0' &&
	     sBit( code, 26 ) == '1' &&
	     sBit( code, 22 ) == '0' &&
	     sBit( code, 20 ) == '0' )
	{
		/* STR */
		inst -> name = STR;
		inst -> op1.val = binToDec( extractSubString( code, 31-15, 31-12 ) );
		inst -> op2.val = binToDec( extractSubString( code, 31-19, 31-16 ) );
		inst -> op1.type = Reg;
		inst -> op2.type = Reg;
		inst -> shift = addressValue( code );
	}
	else
	if ( sBit( code, 27 ) == '1' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 25 ) == '1' &&
	     sBit( code, 24 ) == '0' )
	{
	    inst -> name = B;
	    inst -> op1.val = binToDec( extractSubString( code, 31-23, 31-0 ) );
	    inst -> op1.val = inst -> op1.val << 2;
	    inst -> op1.val = unTC( inst -> op1.val, 26 );
	    inst -> op1.type = Imm;
	    inst -> S = 0;
	}
	else
	if ( sBit( code, 27 ) == '1' &&
	     sBit( code, 26 ) == '0' &&
	     sBit( code, 25 ) == '1' &&
	     sBit( code, 24 ) == '1' )
	{
	    inst -> name = BL;
	    inst -> op1.val = binToDec( extractSubString( code, 31-23, 31-0 ) );
	    inst -> op1.val = inst -> op1.val << 2;
	    inst -> op1.val = unTC( inst -> op1.val, 26 );
	    inst -> op1.type = Imm;
	    inst -> S = 0;
	}
	else
	if ( sBit( code, 27 ) == '1' &&
	     sBit( code, 26 ) == '1' &&
	     sBit( code, 25 ) == '1' &&
	     sBit( code, 24 ) == '1' )
	{
		inst -> name = SVC;
		inst -> op1.val = binToDec( extractSubString( code, 31-23, 31-0 ) );
		inst -> op1.type = Imm;
	}else{
		killProgram( "Instruction not recognised" );
	}

	return inst;
}

/*** System ***/

SystemValues* initSystemValues( Flags flags)
{
	int i;
	SystemValues* output = malloc( sizeof( SystemValues ) );
	output -> reg = malloc( 16 * sizeof( int ) );
	output -> flags = flags;

	for ( i = 0; i < 16; i++ )
	{
		output -> reg[i] = 0;
	}

	return output;
}

void freeInstruction( Instruction* inst )
{
	//printf("Free instruction");
	free( inst -> con );
	free( inst );
}

void excecuteProgram( Source* curPos, SystemValues* vals )
{
    Instruction* instr;
	while ( curPos != NULL )
	{

	    curPos = updatePos( curPos, vals );
	    instr = makeInstruction( curPos -> data );
	    
	    setReg( 15, getReg( 15, vals ) + 4, vals );

		fflush(stdout);
	    if ( !conditionsMet( instr -> con, vals ) )
	    {
	    	//free( instr );
	    	if ( vals -> flags.trace ) 
	    		printf("Condition not met, skipped instruction\n\n");
	    		
	    	continue;
	    }

	    if ( (vals -> flags).trace )
	    {
			printRegisters( vals );
			printf("Next Instruction = %s\n", instructionToString( instr ) );
			printf("At Address = %i\n\n", curPos -> address );
		}



        fflush(stdout);
		if ( executeInstuction( instr, vals ) )
			break;
		
		//free( instr );
	}
}

/*** Read-in file ***/

void read8Chars( char* target, FILE* file )
{
	int i;

	for ( i = 0; i < 8; i++ )
	{
		target[i] =  fgetc( file );
	}

	target[8] = '\0';
}

void checkNextSpace( FILE* file )
{
	if ( fgetc( file ) != ' ' )
		killProgram( intToString((int)fgetc( file )) );
}

int checkNextLine( FILE* file )
{
	char c = fgetc( file );

	if ( c == EOF )
	{
	    printf("checkNextLine = 0");
	    return 0;
	}


	if ( c == '\n' )
	{
		c = fgetc( file );

		if ( c == EOF )
		{
		    printf("checkNextLine = 0");
			return 0;
		}

		ungetc( c, file);
		printf("checkNextLine = 1");
		return 1;
	}

	killProgram( "Expected '\n' or EOF" );
	return 1;
}

Source* newEntry( Source* oldPos )
{
	Source* output = malloc( sizeof( Source ) );
	output -> prev = oldPos;
	output -> next = NULL;

	if ( oldPos != NULL )
		oldPos -> next = output;

	return output;
}

int addData( Source* curPos, FILE *file, int readAll )
{
    int i;
    if ( readAll )
    {
        for ( i = 0; i < 8; i++ )
            curPos -> data[i] = '0';

        curPos -> empty = 1;
        curPos -> changed = 0;
        return 1;
    }
    else
    {
        char c;

        for ( i = 0; i < 9; i++ )
        {
            c = fgetc( file );

            if ( c == -1 )
            {

                for ( i = 0; i < 8; i++ )
                    curPos -> data[i] = '0';

                curPos -> empty = 1;
                curPos -> changed = 0;
                return 1;
            }
        }

        for ( i = 0; i < 8; i++ )
        {
            curPos -> data[i] = fgetc( file );
        }

        curPos -> data[8] = '\0';
        curPos -> empty = 0;
        curPos -> changed = 0;
        fgetc( file );
        return 0;
    }
}

Source* readFile( char* url )
{
	FILE *file = fopen( url, "r" );

	Source* lastPos = NULL;
	Source* curPos;
	Source* first;
    int readAll = 0;

    int i;
    for ( i = 0; i < 1000; i++ )
    {
        curPos = newEntry( lastPos );
        curPos -> address = i * 4;

        if ( i != 0 )
            lastPos -> next = curPos;
        else
            first = curPos;

        readAll = addData( curPos, file, readAll );

        lastPos = curPos;
    }

	return first;
}

Flags getFlags( int argc, char *argv[])
{
	Flags output;
	int i;
	
	output.trace = 0;
	output.before = 0;
	output.after = 0;
	
	for ( i = 0; i < argc; i++ )
	{
		if ( strcmp( argv[i], "-trace") == 0 )
			output.trace = 1;
		else if ( strcmp( argv[i], "-before") == 0 )
			output.before = 1;
		else if ( strcmp( argv[i], "-after") == 0 )
			output.after = 1;
	}
	
	return output;
}



int main( int argc, char *argv[] )
{
	printf("\nThis is an emulator for an ARM processor, designed by Robert Briggs.\n\nRun this program with the arguments -trace, -before and -after. The first prints out the instruction being executed and the current register values. The other two perform a memory dump before and/or after the program is executed. The instruction file (with the extention .emu) but the the last argument.\n\nEnjoy\n\n");

	SystemValues* vals = initSystemValues( getFlags(argc, argv) );
	Source* source = readFile( argv[argc-1] );
	vals -> memory = source;
	setReg( 13, 999, vals);
	if ( vals -> flags.before )
	{
		printf("**** Before Execution Memory Dump ****\n");
		printSource( source );
	}

	if ( vals -> flags.trace )
		printf("**** Executing the code ****\n");
		
	excecuteProgram( source, vals );
	printf("\n\n\n");
	
	if ( vals -> flags.after )
	{
		printf("**** After Execution Memory Dump ****\n");
		printChangedSource( source );
	}
		
	return 0;
}
