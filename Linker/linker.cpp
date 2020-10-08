#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <regex>
#include <set>
using namespace std;

int machineSize = 512;

int line;
int offset;
ifstream infile;
bool nextLine = true;
int lineL = 0;
string inputLine;
map<string, int> symTable;
set<string> doubleSet; // check double defined symbo
map<string, int>::iterator iter;
void createSymBol(string sym, int val, int currentModule, map<string, int> &tempSymTable);
string readSym();
int readInt();
string readIEAR();
int readInstru();
void printMemoryTable(int address, int opcode, int operand, int errorCode, string symForRule3);
void __parseerror(int errcode, int linenum, int lineoffset);
map<string, int> definedModule; //defSym : the module that defined the symbol

int totInstCount = 0;
string symForRule3 = "";
int errorCode = 0;
int useOrNot[16];
map<string, int> defUseOrNot;
int lastOffset;
int blankLine;

struct Token
{
    string token;
    int line;
    int offset;
};
Token getNextToken();

void pass1(string fileName)
{
    int len = 0;
    line = 0;
    offset = 1;
    int currentModule = 0;

    infile.open(fileName);

    //Check file exist
    if (!infile)
    {
        cout << "Could not open file" << endl;
    }

    while (!infile.eof())
    {
        // cout << "~~~~~~~~~~~~newmodule" << endl;
        currentModule++;
        map<string, int> tempSymTable;

        // Part1: definition list(pass1)
        int defCount = readInt();
        if (defCount == -1)
        {
            break;
        }

        // TOO_MANY_DEF_IN_MODULE
        if (defCount >= 16)
        {
            __parseerror(4, line, 0);
        }
        for (int i = 0; i < defCount; i++)
        {
            string sym = readSym();
            if (sym == "__eof__")
            {
                break;
            }

            int val = readInt();
            if (val == -1)
            {
                break;
            }
            createSymBol(sym, val, currentModule, tempSymTable);
        }
        // Part2: uselist (pass1)
        // cout << "~~~~~~~~~~~~Part2: uselist (pass1)" << endl;
        int useCount = readInt();
        if (useCount == -1)
        {
            break;
        }
        // TOO_MANY_USE_IN_MODULE
        if (useCount >= 16)
        {
            __parseerror(5, line, 0);
        }

        for (int i = 0; i < useCount; i++)
        {

            string sym = readSym();
            if (sym == "__eof__")
            {
                break;
            }
        }
        // Part3: instructionlist(pass1)
        int instCount = readInt();
        if (instCount == -1)
        {
            break;
        }
        if (totInstCount + instCount >= machineSize)
        {
            __parseerror(6, line, 0);
        }
        for (int i = 0; i < instCount; i++)
        {
            string addressmode = readIEAR();
            if (addressmode == "__eof__")
            {
                break;
            }
            int instruction = readInstru();
            if (instruction == -1)
            {
                break;
            }
        }
        for (iter = tempSymTable.begin(); iter != tempSymTable.end(); iter++)
        {
            if (iter->second >= instCount)
            {
                cout << "Warning: Module " << currentModule << ": " << iter->first << " too big " << iter->second << " (max=" << instCount - 1 << ") assume zero relative"
                     << endl;
                symTable[iter->first] = 0 + totInstCount;
            }
        }
        totInstCount = totInstCount + instCount;
    }

    // Print symbol table
    cout << "Symbol Table" << endl;
    for (iter = symTable.begin(); iter != symTable.end(); iter++)
    {
        if (doubleSet.count(iter->first))
        {
            cout << iter->first << "=" << iter->second << " Error: This variable is multiple times defined; first value used" << endl;
        }
        else
        {
            cout << iter->first << "=" << iter->second << endl;
        }
    }

    infile.close();
}

void pass2(string fileName)
{
    cout << "Memory Map" << endl;
    int len = 0;
    line = 0;
    offset = 0;
    int currentModule = 0;
    int address;
    totInstCount = 0;

    infile.open(fileName);

    //Check file exist
    if (!infile)
    {
        cout << "Could not open file" << endl;
    }

    while (!infile.eof())
    {
        // cout << "~~~~~~~~~~~~newmodule" << endl;

        //Create module
        currentModule++;
        // Part1: definition list(pass2)
        int defCount = readInt();
        if (defCount == -1)
        {
            break;
        }

        for (int i = 0; i < defCount; i++)
        {
            string sym = readSym();
            if (sym == "__eof__")
            {
                break;
            }
            int val = readInt();
            if (val == -1)
            {
                break;
            }
        }

        // Part2: uselist(pass2)
        int useCount = readInt();

        map<int, string> useMap; // index, use symbol

        if (useCount == -1)
        {
            break;
        }
        for (int i = 0; i < useCount; i++)
        {
            string sym = readSym();
            if (sym == "__eof__")
            {
                break;
            }

            useMap[i] = sym;
            useOrNot[i] = 0;
        }
        // Part3: instructionlist(pass2)
        int instCount = readInt();
        if (instCount == -1)
        {
            break;
        }

        for (int i = 0; i < instCount; i++)
        {
            string addressmode = readIEAR();
            if (addressmode == "__eof__")
            {
                break;
            }
            int instruction = readInstru();
            if (instruction == -1)
            {
                break;
            }

            int opcode = instruction / 1000;
            int operand = instruction % 1000;
            address = totInstCount + i;
            if (addressmode.compare("I") == 0)
            {
                if (instruction >= 10000)
                {
                    opcode = 9;
                    operand = 999;
                    errorCode = 10;
                }
                printMemoryTable(address, opcode, operand, errorCode, symForRule3);
                errorCode = 0;
            }
            else if (addressmode.compare("A") == 0)
            {
                if (operand > machineSize)
                {
                    operand = 0;
                    errorCode = 8;
                }
                printMemoryTable(address, opcode, operand, errorCode, symForRule3);
                errorCode = 0;
            }
            else if (addressmode.compare("R") == 0)
            {
                if (opcode >= 10)
                {
                    opcode = 9;
                    operand = 999;
                    errorCode = 11;
                }
                else
                {
                    if (operand > instCount)
                    {
                        operand = 0;
                        errorCode = 9;
                    }
                    operand = operand + totInstCount;
                }
                printMemoryTable(address, opcode, operand, errorCode, symForRule3);
                errorCode = 0;
            }
            else if (addressmode.compare("E") == 0)
            {
                if (operand >= useCount)
                {
                    errorCode = 6;
                }
                else
                {
                    string useSym = useMap[operand];
                    // if not in symTable
                    if (symTable.count(useSym) == 0)
                    {
                        //mark the symbol in the uselist is used
                        useOrNot[operand] = 1;
                        //not in sybTable, use 0
                        operand = 0;
                        errorCode = 3;
                        symForRule3 = useSym;
                    }
                    else
                    {
                        //mark the symbol in the uselist is used
                        useOrNot[operand] = 1;
                        operand = symTable[useSym];
                        defUseOrNot[useSym] = 1;
                    }
                }
                printMemoryTable(address, opcode, operand, errorCode, symForRule3);
                errorCode = 0;
            }
        }
        totInstCount = totInstCount + instCount;

        //Check rule 7, if symbol in uselist but neer used.
        for (int i = 0; i < useCount; i++)
        {
            if (useOrNot[i] == 0)
            {
                cout << "Warning: Module " << currentModule << ": " << useMap[i] << " appeared in the uselist but was not actually used"
                     << endl;
            }
        }
    }
    cout << endl;
    //Check rule 4,"Warning: Module %d: %s was defined but never used\n"
    for (iter = defUseOrNot.begin(); iter != defUseOrNot.end(); iter++)
    {
        if (iter->second == 0)
        {
            cout << "Warning: Module " << definedModule[iter->first] << ": " << iter->first << " was defined but never used"
                 << endl;
        }
    }
    infile.close();
}

void printWarning(int warningCode, int module)
{
}

void __parseerror(int errcode, int linenum, int lineoffset)
{
    static string errstr[] = {
        "NUM_EXPECTED",           // Number expect
        "SYM_EXPECTED",           // Symbol Expected
        "ADDR_EXPECTED",          // Addressing Expected which is A/E/I/R
        "SYM_TOO_LONG",           // Symbol Name is too long
        "TOO_MANY_DEF_IN_MODULE", // > 16
        "TOO_MANY_USE_IN_MODULE", // > 16
        "TOO_MANY_INSTR",         // total num_instr exceeds memory size (512)
    };

    printf("Parse Error line %d offset %d: %s\n", linenum, lineoffset + 1, errstr[errcode].c_str());
    exit(0);
}

void printMemoryTable(int address, int opcode, int operand, int errorCode, string symForRule3)
{
    string strAddress;
    string strOperand;
    string strInstru;

    // address to string
    if (address < 10)
    {
        strAddress = "00" + to_string(address);
    }
    else if (address < 100)
    {
        strAddress = "0" + to_string(address);
    }
    else if (100 < address)
    {
        strAddress = to_string(address);
    }

    // instruction to string
    if (operand < 10)
    {
        strOperand = "00" + to_string(operand);
    }
    else if (operand < 100)
    {
        strOperand = "0" + to_string(operand);
    }
    else if (100 < operand)
    {
        strOperand = to_string(operand);
    }
    strInstru = to_string(opcode) + strOperand;
    cout << strAddress << ": " << strInstru;
    //print error, errorcode depends on the rule number
    if (errorCode == 0)
    {
        cout << endl;
    }
    else if (errorCode == 3)
    {
        cout << " Error: " << symForRule3 << " is not defined; zero used" << endl;
    }
    else if (errorCode == 6)
    {
        cout << " Error: External address exceeds length of uselist; treated as immediate" << endl;
    }
    else if (errorCode == 8)
    {
        cout << " Error: Absolute address exceeds machine size; zero used" << endl;
    }
    else if (errorCode == 9)
    {
        cout << " Error: Relative address exceeds module size; zero used" << endl;
    }
    else if (errorCode == 10)
    {
        cout << " Error: Illegal immediate value; treated as 9999" << endl;
    }
    else if (errorCode == 11)
    {
        cout << " Error: Illegal opcode; treated as 9999" << endl;
    }
}

int readInt()
{
    // cout << "~~~~~~~~~~~~readInt" << endl;
    Token t = getNextToken();
    string intToken = t.token;
    int linenum = t.line;
    int lineoffset = t.offset;
    // cout << intToken << endl;

    if (intToken == "__eof__")
    {
        return -1;
    }

    for (int i = 0; i < intToken.length(); i++)
    {
        if (!isdigit(intToken[i]))
        {
            __parseerror(0, linenum, lineoffset);
        }
    }
    return stoi(intToken);
}

string readSym()
{
    // cout << "~~~~~~~~~~~~readSym" << endl;
    Token t = getNextToken();
    string sym = t.token;
    string firstCha = sym.substr(0, 1);
    int linenum = t.line;
    int lineoffset = t.offset;
    smatch sm;
    regex reg("[a-zA-Z][a-zA-Z0-9]*");
    // cout << sym << endl;

    // SYM_EXPECTED
    if (!regex_match(firstCha, sm, reg))
    {
        __parseerror(1, linenum, lineoffset);
    }

    if (sym.length() > 16)
    {
        __parseerror(3, linenum, lineoffset);
    }

    return sym;
}

string readIEAR()
{
    // cout << "~~~~~~~~~~~~readIEAR" << endl;
    Token t = getNextToken();
    string IEAR = t.token;
    int linenum = t.line;
    int lineoffset = t.offset;
    regex reg("[IARE]");

    // cout << IEAR << endl;
    if (!regex_match(IEAR, reg))
    {
        __parseerror(2, linenum, lineoffset);
    }

    return IEAR;
}

int readInstru()
{
    // cout << "~~~~~~~~~~~~readInstru:" << endl;
    int i = readInt();
    return i;
}

Token getNextToken()
{
    Token token;
    int tokenStart;
    int tokenEnd;

    // Check if nextLine needed
    while (nextLine)
    {
        if (lineL == 0 && infile.eof())
        {
            token.token = "__eof__";
            token.line = line - 1;
            token.offset = lastOffset;
            return token;
        }
        getline(infile, inputLine);
        lineL = inputLine.length();
        line++;

        //if empty line, continue to get new line
        if (lineL == 0)
        {
            offset = 0;
            blankLine++;
            if (blankLine > 1)
            {
                lastOffset = 0;
            }
            continue;
        }
        nextLine = false;
        offset = 0;
        // Get tokenStart index, check line full of spaces
        while (isspace(inputLine[offset]) && offset < lineL)
        {
            offset++;
        }
        tokenStart = offset;
        if (tokenStart == lineL)
        {
            nextLine = true;
            offset = 0;
        }
    }

    while (isspace(inputLine[offset]) && offset < lineL)
    {
        offset++;
    }
    tokenStart = offset;

    // Get tokenEnd index
    while (!isspace(inputLine[offset]) && offset < lineL)
    {
        offset++;
    }
    tokenEnd = offset;

    // Save token params
    token.token = inputLine.substr(tokenStart, tokenEnd - tokenStart);
    token.line = line;
    token.offset = tokenStart;

    // Move offset to next token, if no token, set nextLine true
    while (isspace(inputLine[offset]) && offset < lineL)
    {
        offset++;
    }
    if (offset >= lineL)
    {
        nextLine = true;
    }
    lastOffset = offset;

    return token;
}

void createSymBol(string sym, int val, int currentModule, map<string, int> &tempSymTable)
{
    if (symTable.count(sym))
    {
        doubleSet.insert(sym);
    }
    else
    {
        symTable[sym] = val + totInstCount;
        tempSymTable[sym] = val; // for rule 5, check if define symbol is bigger than instrcount
        defUseOrNot[sym] = 0;    // 0 means in the symboltable but have not been used.
        definedModule[sym] = currentModule;
    }
}

int main(int argc, char *argv[])
{
    string fileName = argv[1];

    pass1(fileName);

    cout << "" << endl;

    pass2(fileName);

    return 0;
}
