/* 
Вручну реалізувати лексер згідно з вибраним варіантом мови. Після аналізу вхідного тексту лексер
має відобразити його лексичну структуру. Лексичні помилки мають сповіщатися у традиційній
спосіб. Для значимої частини нетривіальних токенів виявлення має здійснювати з
використанням FA-методів. Зберігати значення токенів рекомендується у спеціальній таблиці.

Мова для аналізу php
Мова реалізації C/C++

*/


// keywords взято з https://www.php.net/manual/uk/reserved.keywords.php


// використано Raw string literals, тому можна скопіювати код і легко запустити на якомусь сайті C++ online

#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <iomanip>
#include <algorithm>

using namespace std;

// Типи токенів
enum TokenType {
    PHP_OPEN,           // <?php
    PHP_CLOSE,          // ?>
    VARIABLE,           // $variable
    INTEGER,            // 1234
    FLOAT,              // 12.34
    STRING,             // "string" або 'string'
    IDENTIFIER,         // назви функцій, класів
    KEYWORD,            // if, else, while, function ...
    OPERATOR,           // +, -, *, /, =, ==, !=
    DELIMITER,          // ; , ( ) { }
    WHITESPACE,         // пробіли
    COMMENT             // // або /* */
};

// Структура токена
struct Token {
    TokenType type;
    string value;
    int line;
    int column;
    
    Token(TokenType t, const string& v, int l, int c) : type(t), value(v), line(l), column(c) {}
};

// Таблиця токенів для зберігання
class TokenTable {
private:
    vector<Token> tokens;
    
public:
    void addToken(const Token& token) {
        tokens.push_back(token);
    }
    
    void printTable() {
        cout << "\n=== ТАБЛИЦЯ ТОКЕНІВ ===\n";
        cout << left << setw(15) << "ТИП" << setw(20) << "ЗНАЧЕННЯ" 
             << setw(15) << "РЯДОК" << setw(15) << "СТОВПЕЦЬ" << endl;
        cout << string(51, '-') << endl;
        
        for (const auto& token : tokens) {
            cout << left << setw(15) << getTokenTypeName(token.type)
                 << setw(20) << token.value
                 << setw(8) << token.line
                 << setw(8) << token.column << endl;
        }
    }
    
    
private:
    string getTokenTypeName(TokenType type) {
        switch (type) {
            case PHP_OPEN: return "PHP_OPEN";
            case PHP_CLOSE: return "PHP_CLOSE";
            case VARIABLE: return "VARIABLE";
            case INTEGER: return "INTEGER";
            case FLOAT: return "FLOAT";
            case STRING: return "STRING";
            case IDENTIFIER: return "IDENTIFIER";
            case KEYWORD: return "KEYWORD";
            case OPERATOR: return "OPERATOR";
            case DELIMITER: return "DELIMITER";
            case COMMENT: return "COMMENT";
            default: return "OTHER";
        }
    }
};

class PHPLexer {
private:
    string input;
    size_t position;
    int line;
    int column;
    TokenTable tokenTable;
    vector<string> errors;
    
    // Ключові слова PHP https://www.php.net/manual/uk/reserved.keywords.php
	vector<string> keywords = {
		"__halt_compiler()", "abstract", "and", "array()", "as",
		"break", "callable", "case", "catch", "class",
		"clone", "const", "continue", "declare", "default",
		"die()", "do", "echo", "else", "elseif",
		"empty()", "enddeclare", "endfor", "endforeach", "endif",
		"endswitch", "endwhile", "eval()", "exit()", "extends",
		"final", "finally", "fn", "for", "foreach",
		"function", "global", "goto", "if", "implements",
		"include", "include_once", "instanceof", "insteadof", "interface",
		"isset()", "list()", "match", "namespace", "new",
		"or", "print", "private", "protected", "public",
		"readonly", "require", "require_once", "return", "static",
		"switch", "throw", "trait", "try", "unset()",
		"use", "var", "while", "xor", "yield",
		"yield from"
	};
    
public:
    PHPLexer(const string& code) : input(code), position(0), line(1), column(1) {}
    
    void analyze() {
        while (position < input.length()) {
            skipWhitespace();
            
            if (position >= input.length()) break;
            
            char current = input[position];
            
            // Перевірка на <?php
            if (current == '<' && position + 4 < input.length() && 
                input.substr(position, 5) == "<?php") {
                addToken(PHP_OPEN, "<?php", 5);
            }
            // Перевірка на ?>
            else if (current == '?' && position + 1 < input.length() && 
                     input[position + 1] == '>') {
                addToken(PHP_CLOSE, "?>", 2);
            }
            // Змінні (починаються з $)
            else if (current == '$') {
                readVariable();
            }
            // Числа
            else if (isdigit(current)) {
                readNumber();
            }
            // Рядки
            else if (current == '"' || current == '\'') {
                readString();
            }
            // Коментарі
            else if (current == '/' && position + 1 < input.length()) {
                if (input[position + 1] == '/') {
                    readSingleLineComment();
                } else if (input[position + 1] == '*') {
                    readMultiLineComment();
                } else {
                    addToken(OPERATOR, "/", 1);
                }
            }
            // Оператори
            else if (isOperator(current)) {
                readOperator();
            }
            // Роздільники
            else if (isDelimiter(current)) {
                addToken(DELIMITER, string(1, current), 1);
            }
            // Ідентифікатори та ключові слова
            else if (isalpha(current) || current == '_') {
                readIdentifier();
            }
            // Невідомий символ
            else {
                addError("Невідомий символ: " + string(1, current));
                advance();
            }
        }
        
        printResults();
    }
    
private:
    char peek(int offset = 0) {
        size_t pos = position + offset;
        return input[pos];
    }
    
    void advance() {
        if (position < input.length()) {
            if (input[position] == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            position++;
        }
    }
    
    void skipWhitespace() {
        while (position < input.length() && isspace(input[position])) {
            advance();
        }
    }
    
    void addToken(TokenType type, const string& value, int length) {
        tokenTable.addToken(Token(type, value, line, column));
        for (int i = 0; i < length; i++) {
            advance();
        }
    }
    
    void addError(const string& error) {
        errors.push_back("Помилка на рядку " + to_string(line) + 
                        ", стовпець " + to_string(column) + ": " + error);
        advance();
    }
    
	// Читання змінної
    void readVariable() {
        int startColumn = column;
        string value = "$";
        advance();
        
        if (!isalpha(peek()) && peek() != '_') {
            addError("Неправильна назва змінної");
            return;
        }
        
        while (position < input.length() && 
               (isalnum(input[position]) || input[position] == '_')) {
            value += input[position];
            advance();
        }
        
        tokenTable.addToken(Token(VARIABLE, value, line, startColumn));
    }
    
    // Читання числа
    void readNumber() {
        int startColumn = column;
        string value = "";
        bool isFloat = false;
        
        while (position < input.length() && isdigit(input[position])) {
            value += input[position];
            advance();
        }
        
        if (position < input.length() && input[position] == '.') {
            if (position + 1 < input.length() && isdigit(input[position + 1])) {
                isFloat = true;
                value += input[position];
                advance();
                
                while (position < input.length() && isdigit(input[position])) {
                    value += input[position];
                    advance();
                }
            }
        }
        
        TokenType type = isFloat ? FLOAT : INTEGER;
        tokenTable.addToken(Token(type, value, line, startColumn));
    }
    
    // Читання рядка
    void readString() {
        int startColumn = column;
        char quote = input[position];
        string value = "";
        value += quote;
        advance();
        
        while (position < input.length() && input[position] != quote) {
            if (input[position] == '\\' && position + 1 < input.length()) {
                value += input[position];
                advance();
                value += input[position];
                advance();
            } else {
                value += input[position];
                advance();
            }
        }
        
        if (position >= input.length()) {
            addError("Незакритий рядок");
            return;
        }
        
        value += input[position];
        advance();
        
        tokenTable.addToken(Token(STRING, value, line, startColumn));
    }
    
    // Читання однорядкового коментаря
    void readSingleLineComment() {
        int startColumn = column;
        string value = "";
        
        while (position < input.length() && input[position] != '\n') {
            value += input[position];
            advance();
        }
        
        tokenTable.addToken(Token(COMMENT, value, line, startColumn));
    }
    
    // Читання багаторядкового коментаря
    void readMultiLineComment() {
        int startColumn = column;
        string value = "";
        value += input[position]; 
        advance();
        value += input[position]; 
        advance();
        
        while (position + 1 < input.length()) {
            if (input[position] == '*' && input[position + 1] == '/') {
                value += input[position];
                advance();
                value += input[position];
                advance();
                tokenTable.addToken(Token(COMMENT, value, line, startColumn));
                return;
            }
            value += input[position];
            advance();
        }
        
        addError("Незакритий коментар");
    }
    
    // Читання ідентифікатора
    void readIdentifier() {
        int startColumn = column;
        string value = "";
        
        while (position < input.length() && 
               (isalnum(input[position]) || input[position] == '_')) {
            value += input[position];
            advance();
        }
        
        TokenType type = find(keywords.begin(), keywords.end(), value) != keywords.end() ? KEYWORD : IDENTIFIER;
        tokenTable.addToken(Token(type, value, line, startColumn));
    }
    
    // Читання оператора
    void readOperator() {
        int startColumn = column;
        string value = "";
        value += input[position];
        
        // Перевіряємо на двосимвольні оператори
        if (position + 1 < input.length()) {
            string twoChar = value + input[position + 1];
            if (twoChar == "==" || twoChar == "!=" || twoChar == "<=" || 
                twoChar == ">=" || twoChar == "++" || twoChar == "--" ||
                twoChar == "+=" || twoChar == "-=" || twoChar == "*=" ||
                twoChar == "/=" || twoChar == "&&" || twoChar == "||") {
                value = twoChar;
                advance();
            }
        }
        
        advance();
        tokenTable.addToken(Token(OPERATOR, value, line, startColumn));
    }
    
    bool isOperator(char c) {
        return c == '+' || c == '-' || c == '*' || c == '/' || c == '=' ||
               c == '<' || c == '>' || c == '!' || c == '&' || c == '|' ||
               c == '%' || c == '^' || c == '~';
    }
    
    bool isDelimiter(char c) {
        return c == ';' || c == ',' || c == '(' || c == ')' || 
               c == '{' || c == '}' || c == '[' || c == ']' ||
               c == ':' || c == '.' || c == '?';
    }
    
    void printResults() {
        tokenTable.printTable();
        
        if (!errors.empty()) {
            cout << "\n=== ПОМИЛКИ ===\n";
            for (const auto& error : errors) {
                cout << error << endl;
            }
        } else {
            cout << "\n=== ПОМИЛОК НЕ ЗНАЙДЕНО ===\n";
        }
    }
};

int main() {
    // Тестовий PHP код
    // Взятий розв'язок однієї з задач на LeetCode
    // https://github.com/php4dev/LeetCode-in-Php/blob/main/src/Algorithms/s0130_surrounded_regions/Solution.php
    // також я додав @ та /* щоби були помилки для прикладу 
    // приклад помилок
    // Помилка на рядку 56, стовпець 11: Невідомий символ: @
    // Помилка на рядку 58, стовпець 1: Незакритий коментар
    // використано Raw string literals, тому можна скопіювати код і легко запустити на якомусь сайті C++ online online compiler and debugger for c/c++
    string phpCode = R"(<?php

class Solution {

    /**
     * @param String[][] $board
     * @return NULL
     */
    function solve(&$board) {
        if (($board == null) || count($board) == 0 || count($board[0]) == 0)
            return;
        for ($row = 0; $row < count($board); $row++) {
            for ($col = 0; $col < count($board[0]); $col++) {
                if (($row == 0) || ($row == count($board) - 1) || ($col == 0) || ($col == count($board[0]) - 1)) {
                    if ($board[$row][$col] == 'O') {
                        self::color($board, $row, $col);
                    }
                }
            }
        }

        for ($row = 0; $row < count($board); $row++) {
            for ($col = 0; $col < count($board[0]); $col++) {

                if ($board[$row][$col] == 'O') {
                    $board[$row][$col] = 'X';
                }
                if ($board[$row][$col] == 'V') {
                    $board[$row][$col] = 'O';
                }
            }
        }
    }

    function color(&$board, $row, $col) {
        $board[$row][$col] = 'V';
        if ($row + 1 < count($board)) {
            if ($board[$row + 1][$col] == 'O') {
                self::color($board, $row + 1, $col);
            }
        }
        if ($row - 1 >= 0) {
            if ($board[$row - 1][$col] == 'O') {
                self::color($board, $row - 1, $col);
            }
        }
        if ($col + 1 < count($board[0])) {
            if ($board[$row][$col + 1] == 'O') {
                self::color($board, $row, $col + 1);
            }
        }
        if ($col - 1 >= 0) {
            if ($board[$row][$col - 1] == 'O') {
                self::color($board, $row, $col - 1);
            }
        } @
    } /*
})";

    cout << "=== Лексичний аналізатор для мови PHP ===\n";
    cout << "Вхідний код:\n" << phpCode << "\n";
    cout << string(50, '=') << endl;
    
    PHPLexer lexer(phpCode);
    lexer.analyze();
    
    return 0;
}
