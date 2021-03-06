# junkbasic

Trying for a small self-hosted BASIC native code compiler for the P2.

## Editor Commands

    NEW
    NEW filename
    LOAD
    LOAD filename
    SAVE
    SAVE filename
    LIST
    RUN
    RENUM

## Language syntax

### Comments

    REM comment

### Function Definitions

    FUNCTION function-name
    FUNCTION function-name ( arg [ , arg ]... )

    END FUNCTION

### Subroutine Defintions

    SUB subroutine-name
    SUB subroutine-name  ( arg [ , arg ]... )

    END SUB

### Variable Declarations

    dim-statement:

        DIM variable-def [ , variable-def ]...
    
    variable-def:

        var [ scalar-initializer ]
        var '[' size ']' [ array-initializer ]
    
    scalar-initializer:

        = constant-expr
    
    array-initializer:

        = { constant-expr [ , constant-expr ]... }

### Assignment Statements

    [LET] var = expr

### Control Statements

    RETURN expr

    IF expr

    ELSE IF expr

    ELSE

    END IF

    END

    FOR var = start TO end [ STEP inc ]

    NEXT var

    DO
    DO WHILE expr
    DO UNTIL expr

    LOOP
    LOOP WHILE expr
    LOOP UNTIL expr

    label:

    GOTO label

### Output Statements

    PRINT expr [ ;|, expr ]... [ ; ]

### Expressions

    expr AND expr
    expr OR expr

    expr ^ expr
    expr | expr
    expr & expr

    expr = expr
    expr <> expr

    expr < expr
    expr <= expr
    expr >= expr
    expr > expr

    expr << expr
    expr >> expr

    expr + expr
    expr - expr
    expr * expr
    expr / expr
    expr MOD expr

    - expr
    ~ expr
    NOT expr

    function ( arg [, arg ]... )
    array [ index ]

    (expr)
    var
    integer
    "string"

## Built-in Variables

    clkfreq
    par
    cnt
    ina
    inb
    outa
    outb
    dira
    dirb
    ctra
    ctrb
    frqa
    frqb
    phsa
    phsb
    vcfg
    vscl

## Built-in Functions

    waitcnt(target)
    waitpeq(state, mask)
    waitpne(state, mask)

