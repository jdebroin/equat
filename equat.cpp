#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#ifndef _WIN32
#include <readline/readline.h>
#include <readline/history.h>
#else // !_WIN32
inline char* readline( const char* prompt )
{
    char* buffer = (char*) malloc( 1024 );
    fgets( buffer, 1024, stdin );
    return buffer;
}
#endif // !_WIN32

typedef long double EquatValue;

class CName
{
public:
    CName()
    {
        *m_name = 0;
    }
    virtual ~CName()
    {
    }
    bool isDefined()
    {
        return (*m_name != 0);
    }
    void define(const char* name)
    {
        strcpy( m_name, name );
    }
    bool is(const char* name)
    {
        return (strcmp( m_name, name ) == 0);
    }
protected:
    char m_name[80];
};

class CVariable : public CName
{
public:
    CVariable()
    {
        m_value = 0.0;
        m_isConst = false;
    }
    virtual ~CVariable()
    {
    }
    void define(const char* name, EquatValue value = 0.0, bool isConst = false)
    {
        CName::define( name );
        m_value = value;
        m_isConst = isConst;
    }
    bool set(EquatValue value)
    {
        if (m_isConst)
            return false;
        m_value = value;
        return true;
    }
    void get(EquatValue* value)
    {
        *value = m_value;
    }
protected:
    EquatValue m_value;
    bool m_isConst;
};

class CFunction : public CName
{
public:
    CFunction()
    {
        m_nbParams = 0;
        m_params = 0;
    }
    virtual ~CFunction()
    {
        if (m_params)
            delete [] m_params;
    }
    void define(const char* name, int nbParams)
    {
        CName::define( name );
        m_nbParams = nbParams;
        if (m_params)
            delete [] m_params;
        m_params = new EquatValue[m_nbParams];
        for (int i = 0; i < m_nbParams; i++)
        {
            m_params[i] = 0.0;
        }
    }
    int getNbParams()
    {
        return m_nbParams;
    }
    void setParam(int i, EquatValue value)
    {
        if (i >= 0 && i < m_nbParams)
        {
            m_params[i] = value;
        }
    }
    virtual EquatValue eval() = 0;
protected:
    int m_nbParams;
    EquatValue* m_params;
};

class CUnaryFunction : public CFunction
{
public:
    CUnaryFunction( const char* name, double (*f) (double p))
    {
        define( name, 1 );
        m_f = f;
    }
    virtual EquatValue eval()
    {
        return m_f( m_params[0] );
    }
protected:
    double (*m_f) (double p);
};

class CParser
{
public:
    CParser();
    virtual ~CParser();

    void init(const char* expr);
    
    void evalExpr();

    const char* getError()
    {
        return m_error;
    }
    EquatValue getResult()
    {
        return m_result;
    }
    void display(int base);

protected:
    void evalExpr1(EquatValue *answer);
    void evalExpr2(EquatValue *answer);
    void evalExpr3(EquatValue *answer);
    void evalExpr4(EquatValue *answer);
    void evalExpr5(EquatValue *answer);
    void evalExpr6(EquatValue *answer);
    void atom(EquatValue *answer);
    void getToken(void), putback(void);
    void unary(char o, EquatValue *r);
    void setError(const char* msg, int line);
    void getVarValue(EquatValue* answer);
    bool isdelim(char c);
    bool setVar(const char* name, EquatValue value);
    
    const char* m_expr;      // expression a analyser
    const char* m_sav;       // position de depart
    EquatValue      m_result;    // resultat du calcul
    const char* m_error;     // code d'erreur
    int         m_errorLine;
    char        m_token[80];
    char        m_tokType;

    int         m_nbMaxVars;
    int         m_nbVars;
    CVariable*  m_vars;      // variables
    int         m_nbMaxFunctions;
    int         m_nbFunctions;
    CFunction**  m_functions;

    enum 
    {
        DELIMITER = 1,
        NAME = 2,
        NUMBER = 3
    };
};

CParser::CParser()
    :m_expr  ( 0 ),
     m_sav   ( 0 ),
     m_result( 0 ),
     m_error ( 0 )
{
    m_nbMaxVars = 100;
    m_vars = new CVariable[m_nbMaxVars];
    m_nbVars = 0;
    m_vars[m_nbVars++].define( "pi", M_PI, true );
    m_vars[m_nbVars++].define( "e", M_E, true );
    
    m_nbMaxFunctions = 100;
    m_functions = new CFunction*[m_nbMaxFunctions];
    m_nbFunctions = 0;
    m_functions[m_nbFunctions++] = new CUnaryFunction( "sqrt", sqrt );
    m_functions[m_nbFunctions++] = new CUnaryFunction( "log", log );
    m_functions[m_nbFunctions++] = new CUnaryFunction( "exp", exp );
    m_functions[m_nbFunctions++] = new CUnaryFunction( "log10", log10 );
    m_functions[m_nbFunctions++] = new CUnaryFunction( "sin", sin );
    m_functions[m_nbFunctions++] = new CUnaryFunction( "cos", cos );
    m_functions[m_nbFunctions++] = new CUnaryFunction( "tan", tan );
    m_functions[m_nbFunctions++] = new CUnaryFunction( "asin", asin );
    m_functions[m_nbFunctions++] = new CUnaryFunction( "acos", acos );
    m_functions[m_nbFunctions++] = new CUnaryFunction( "atan", atan );
}

CParser::~CParser()
{
    if (m_functions)
    {
        int i;
        for (i = 0; i < m_nbFunctions; i++)
        {
            delete m_functions[i];
        }
        delete [] m_functions;
    }
    if (m_vars)
        delete [] m_vars;
}

void CParser::init(const char* expr)
{
    m_expr   = expr;
    m_sav    = expr;
    m_result = 0;
    m_error  = 0;
}

// Point d'entree du parser
void CParser::evalExpr()
{
    getToken();
    if (!*m_token) 
    {
        setError( "aucun token", __LINE__ );
        return;
    }
    evalExpr1( &m_result );
}

// signe =
void CParser::evalExpr1(EquatValue *answer)
{
    char tempToken[80];

    if (m_tokType == NAME) 
    {
        // sauve l'ancien element (nom de variable)
        strcpy(tempToken, m_token);
        
        getToken();
        if (*m_token != '=') 
        {
            // pas assignation
            // parse normalement
            m_expr = m_sav;
            getToken();
        }
        else 
        {
            // assignation
            getToken(); // va chercher la 2e partie de l'exp
            evalExpr2(answer);
            setVar( tempToken, *answer );
            return;
        }
    }
    
    evalExpr2(answer);
}

// Ajoute ou soustrait des operandes
void CParser::evalExpr2(EquatValue *answer)
{
    register char op;
    EquatValue temp;
    
    evalExpr3(answer);
    while ((op = *m_token) == '+' || op == '-') 
    {
        getToken();
        evalExpr3(&temp);
        switch(op) {
            case '-':
                *answer = *answer - temp;
                break;
            case '+':
                *answer = *answer + temp;
                break;
        }
    }
}

// multiplie ou divise deux facteurs
void CParser::evalExpr3(EquatValue *answer)
{
    register char op;
    EquatValue temp;
    evalExpr4(answer);
    while ((op = *m_token) == '*' || op == '/' || op == '%') 
    {
        getToken();
        evalExpr4(&temp);
        switch (op)
        {
            case '*':
                *answer = *answer * temp;
                break;
            case '/':
                *answer = *answer / temp;
                break;
            case '%':
                *answer = (short) *answer % (short) temp;
                break;
        }
    }
}

// s'occupe d'un exposant
void CParser::evalExpr4(EquatValue *answer)
{
    EquatValue temp, ex;
    evalExpr5(answer);
    if (*m_token == '^') 
    {
        getToken();
        evalExpr4(&temp);
        ex = *answer;
        *answer = pow( ex, temp );
    }
}

// Evalue un + ou un - (unaire)
void CParser::evalExpr5(EquatValue *answer)
{
    register char op;
    op = 0;
    if ((m_tokType == DELIMITER) && *m_token == '+' || *m_token == '-') 
    {
        op = *m_token;
        getToken();
    }
    evalExpr6(answer);
    if (op=='-') *answer = -(*answer);
}

// Traite les parentheses
void CParser::evalExpr6(EquatValue *answer)
{
    if (*m_token == '(') 
    {
        getToken();
        evalExpr2(answer);
        if (*m_token != ')')
            setError( "manque ')'", __LINE__ );
        getToken();
    }
    else atom(answer);
}

// traite un element elementaire
void CParser::atom(EquatValue *answer)
{
    EquatValue	x;
    
    switch(m_tokType) 
    {
        case NAME:
            getVarValue( answer );
            getToken();
            return;
        case NUMBER:
            x = strtod( m_token, NULL );
            *answer = x;
            getToken();
            return;
        default:
            setError( "atome non reconnu", __LINE__ );
    }
}

// retourne un element a la chaine a analyser
void CParser::putback(void)
{
    char *t;
    
    t = m_token;
    for (; *t; t++) m_expr--;
}

// Retourne le prochain element
void CParser::getToken(void)
{
    register char *temp;
    m_tokType = 0;
    temp = m_token;
    *temp = '\0';
    
    if (*m_expr == 0) return; // fin de l'expression
    
    while (isspace(*m_expr)) ++m_expr; // saute les espaces
    
    if (isdelim( *m_expr )) 
    {
        m_tokType = DELIMITER;
        // avance au prochain element
        *temp++ = *m_expr++;
    }
    else if (isalpha(*m_expr)) 
    {
        while (!isdelim(*m_expr)) *temp++ = *m_expr++;
        m_tokType = NAME;
    }
    else if (isdigit(*m_expr) || *m_expr == '.') 
    {
        while(!isdelim(*m_expr)) *temp++ = *m_expr++;
        m_tokType = NUMBER;
    }
    
    *temp = '\0';
}

// verifie si c est delimiteur
bool CParser::isdelim(char c)
{
    if (strchr(" +-/*%^=(),", c) || c==9 || c =='\r' || c==0)
        return true;
    return false;
}

void CParser::getVarValue(EquatValue *answer)
{
    int i;
    for (i = 0; i < m_nbFunctions; i++)
    {
        CFunction* function = m_functions[i];
        if (function->is( m_token ))
        {
            getToken();
            if (*m_token != '(')
            {
                setError( "manque '('", __LINE__ );
                return;
            }

            getToken();

            int nbP = function->getNbParams();
            for (int i = 0; i < nbP; i++)
            {
                if (*m_token == ')')
                {
                    // need more params
                    setError( "manque des parametres", __LINE__ );
                    return;
                }
                EquatValue value;
                evalExpr2( &value );
                function->setParam( i, value );
                if (i < nbP - 1)
                {
                    if (*m_token != ',')
                    {
                        setError( "manque des parametres", __LINE__ );
                        return;
                    }
                    getToken();
                }
            }

            if (*m_token != ')')
            {
                setError( "manque ')'", __LINE__ );
                return;
            }
            *answer = function->eval();
            return;
        }
    }
    
    *answer = 0.0;
    for (i = 0; i < m_nbVars; i++)
    {
        if (m_vars[i].is( m_token ))
        {
            m_vars[i].get( answer );
            break;
        }
    }
}

bool CParser::setVar(const char* name, EquatValue value)
{
    for (int i = 0; i < m_nbVars; i++)
    {
        if (m_vars[i].is( name ))
        {
            if (!m_vars[i].set( value ))
            {
                setError( "assignation a une constante", __LINE__ );
                return false;
            }
            return true;
        }
    }
    for (int i = 0; i < m_nbVars; i++)
    {
        if (!m_vars[i].isDefined())
        {
            m_vars[i].define( name, value );
            return true;
        }
    }
    if (m_nbVars < m_nbMaxVars)
    {
        m_vars[m_nbVars].define( name, value );
        m_nbVars++;
        return true;
    }
    setError( "plus de variables", __LINE__ );
    return false;
}

// assigne un message d'erreur
void CParser::setError(const char* error, int line)
{
    if (m_error == 0) 
    {
        m_error = error;
        m_errorLine = line;
    }
}

void CParser::display(int base)
{
    if (getError())
    {
        printf( "Erreur: %s a la ligne %d\n", getError(), m_errorLine );
        return;
    }

    if (base == 16)
    {
        unsigned long result = (unsigned long) getResult();
        printf( "  0x%x\n", result );
    }
    else if (base == 8)
    {
        unsigned long result = (unsigned long) getResult();
        printf( "  0%o\n", result );
    }
    else if (base == 2)
    {
        unsigned long result = (unsigned long) getResult();
        char s[33];
        int i;
        for (i = 0; i < 32; i++)
        {
            s[i] = ((result >> (32 - i - 1)) & 0x00000001) ? '1' : '0';
        }
        s[i] = 0;
        printf( "  %s\n", s );
    }
    else
    {
        printf( "  %.16Lg\n", getResult() );
    }
}

void usage()
{
    printf( "usage: equat [-h|-b|-o]\n" );
}
            
int main(int argc, char* argv[])
{
    CParser parser;
   
    int base = 10;
    bool interactive = true;

    int i;
    for (i = 1; i < argc; i++)
    {
        if (*(argv[i]) == '-' )
        {
            switch (*(argv[i] + 1))
            {
            case 'x':
                base = 16;
                break;
            case 'b':
                base = 2;
                break;
            case 'o':
                base = 8;
                break;
            case 'i':
                interactive = false;
                break;
            default:
                usage();
                return 0;
            }
        }
    }

    char* expr = 0;

    while (true)
    {
        if (expr)
        {
            free( expr );
            expr = 0;
        }
        if (interactive)
        {
            expr = readline( "> " );
            if (expr == 0)
                break;
            add_history( expr );
        }
        else
        {
            expr = (char*) malloc( 1024 );
            fgets( expr, 1024, stdin );
        }
        
        if (*expr == '\n' || *expr == 0)
        {
            break;
        }

        parser.init( expr );
        parser.evalExpr();
        parser.display( base );

        if (!interactive) break;
    }

    if (expr)
    {
        free( expr );
        expr = 0;
    }
    
    return 0;
}
