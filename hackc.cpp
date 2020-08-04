//------------------------------------------------------------------------------
/// @file       hackc.cpp
/// @author     Milos Milicevic (milosh.mkv@gmail.com)
/// @brief      Hack assembly compiler.
/// @version    0.1
/// @date       2020-08-04
///
/// @copyright  Copyright (c) 2020
///
/// Distributed under the MIT software license, see the accompanying
/// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//------------------------------------------------------------------------------

#include <iostream>
#include <map>
#include <fstream>
#include <vector>
#include <regex>
#include <algorithm>

static std::map<std::string, unsigned> SYMBOLS = {
    { "R0",         0 }, { "R1",      1 }, { "R2",   2 }, { "R3",   3 }, { "R4",   4 }, { "R5",    5 }, 
    { "R6",         6 }, { "R7",      7 }, { "R8",   8 }, { "R9",   9 }, { "R10", 10 }, { "R11",  11 },
    { "R12",        2 }, { "R13",    13 }, { "R14", 14 }, { "R15", 15 }, { "THAT", 4 }, { "THIS",  3 },
    { "SCREEN", 16384 }, { "KBD", 24576 }, { "SP",   0 }, { "LCL",  1 }, { "ARG",  2 }
};

static std::map<std::string, std::string> DESTINATIONS = {
    { "NULL", "000" }, { "M",  "001" }, { "D",  "010" }, { "MD",  "011" }, 
    { "A",    "100" }, { "AM", "101" }, { "AD", "110" }, { "AMD", "111" }
};

static std::map<std::string, std::string> JUMPS = {
    { "NULL", "000" }, { "JGT", "001" }, { "JEQ", "010" }, { "JGE", "011" }, 
    { "JLT",  "100" }, { "JNE", "101" }, { "JLE", "110" }, { "JMP", "111" }
};

static std::map<std::string, std::string> COMPUTATIONS = {
    { "0",   "0101010" }, { "1",   "0111111" }, { "-1",  "0111010" }, { "D",   "0001100" }, 
    { "A",   "0110000" }, { "!D",  "0001101" }, { "!A",  "0110001" }, { "-D",  "0001111" }, 
    { "-A",  "0110011" }, { "D+1", "0011111" }, { "A+1", "0110111" }, { "D-1", "0001110" },
    { "A-1", "0110010" }, { "D+A", "0000010" }, { "D-A", "0010011" }, { "A-D", "0000111" }, 
    { "D&A", "0000000" }, { "D|A", "0010101" }, { "M",   "1110000" }, { "!M",  "1110001" }, 
    { "-M",  "1110011" }, { "M+1", "1110111" }, { "M-1", "1110010" }, { "D+M", "1000010" },
    { "D-M", "1010011" }, { "M-D", "1000111" }, { "D&M", "1000000" }, { "D|M", "1010101" }, 
    { "M+D", "1000010" }
};

static unsigned NEXT_SYMBOL_VALUE = 16;

//------------------------------------------------------------------------------
/// @brief              Load hack assembly file and return its code in vector.
///
/// @param hack_file    Path to file.
/// @return             std::vector<std::string> 
static std::vector<std::string> Load_Hack_Code(const std::string& hack_file)
{
    std::ifstream hack_file_stream(hack_file);
    std::string line;
    std::vector<std::string> file_data;
    while(std::getline(hack_file_stream, line))
        file_data.push_back(line);
    return file_data;
}

//------------------------------------------------------------------------------
/// @brief              Add new symbol to SYMBOLS map.
///
/// @param symbol       Symbol name.
/// @param value        Symbol value.
static void Add_New_Symbol(const std::string& symbol, long int value = -1)
{
    if(SYMBOLS.count(symbol) > 0) return;

    if(value > -1)
        SYMBOLS.insert({symbol, value});
    else
        SYMBOLS.insert({symbol, NEXT_SYMBOL_VALUE++});
}

//------------------------------------------------------------------------------
/// @brief           Split string.
///
/// @param str       String to split.
/// @param delim     Delimiter.
/// @return          std::vector<std::string> 
static std::vector<std::string> Split_String(const std::string str, const std::string& delim)
{
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    return tokens;
}

//------------------------------------------------------------------------------
/// @brief              Find all labels and add them to symbols.
///
/// @param hack_code    Loaded hack code.
static void Process_Labels(std::vector<std::string> hack_code)
{
    unsigned PC = 0;
    for(std::string& line : hack_code)
    {
        line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
        if(std::regex_match(line, std::regex("\\/\\/.+$")) || line.empty())
        {
            continue;
        }
        line = Split_String(line, "//")[0];
        if(std::regex_match(line, std::regex("(\\([a-zA-Z.:$_]+\\)$|\\([a-zA-Z.:$_][a-zA-Z0-9.:$_]+\\)$)")))
        {
            Add_New_Symbol(line.substr(1, line.size()-2), PC);
            continue;
        }    
        PC++;    
    }
}

//------------------------------------------------------------------------------
/// @brief          Check if string can be parsed to integer.
///
/// @param str      String to check.
/// @return         bool 
static bool Is_String_Number(const std::string& str)
{
    for (int i = 0; i < str.size(); i++) 
        if (isdigit(str[i]) == false)
            return false;
    return true;
}

static std::vector<std::string> BINARY;

//------------------------------------------------------------------------------
/// @brief              Create binary from code.
///
/// @param hack_code    Loaded hack code.
void Compile_Hack_Code(std::vector<std::string>& hack_code)
{
    int line_number = 1;
    for(std::string& line : hack_code)
    {
        std::string fill_line = line;
        line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());

        if(std::regex_match(line, std::regex("\\/\\/.+$")) || line.empty() || std::regex_match(line, std::regex("(\\([a-zA-Z.:$_]+\\)$|\\([a-zA-Z.:$_][a-zA-Z0-9.:$_]+\\)$)")))
        {
            line_number += 1;
            continue;
        }
        line = Split_String(line, "//")[0];
        if(std::regex_match(line, std::regex("(@[a-zA-Z.:$_]+$|@[a-zA-Z.:$_][a-zA-Z0-9.:$_]+$|@[0-9]+$)")))
        {
            std::string symbol = line.substr(1, line.size());
            unsigned number;
            if(Is_String_Number(symbol)) number = std::atoi(symbol.c_str());
            else
            {
                Add_New_Symbol(symbol);
                number = SYMBOLS[symbol];
            }
            std::string binary = "0" + std::bitset<15>(number).to_string();
            BINARY.push_back(binary);
            line_number += 1;
        }
        else
        {
            std::string destination, computation, jump;
            if(std::regex_match(line, std::regex(".+=.+;.+")))
            {
                destination = Split_String(line, "=")[0];
                std::string splits = Split_String(line, "=")[1];
                computation = Split_String(splits, ";")[0];
                jump = Split_String(splits, ";")[1];
            }
            else if(std::regex_match(line, std::regex(".+=.+")))
            {
                std::vector<std::string> splits = Split_String(line, "=");
                destination = splits[0];
                computation = splits[1];
            }
            else if(std::regex_match(line, std::regex(".+;.+")))
            {
                std::vector<std::string> splits = Split_String(line, ";");
                computation = splits[0];
                jump = splits[1];
            }
            else throw ("Error on line: " + std::to_string(line_number) + " => " + fill_line);

            std::string destination_bin = destination.empty() ? DESTINATIONS["NULL"] : DESTINATIONS[destination];
            std::string computation_bin = computation.empty() ? COMPUTATIONS["NULL"] : COMPUTATIONS[computation];
            std::string jump_bin        =        jump.empty() ?        JUMPS["NULL"] :        JUMPS[jump];
            if(destination_bin.empty() || computation_bin.empty() || jump_bin.empty())
                throw ("Error on line: " + std::to_string(line_number) + " => " + fill_line);
            std::string binary = "111" + computation_bin + destination_bin + jump_bin;
            BINARY.push_back(binary);
            line_number += 1;
        }
    }
}

int main(int argc, const char ** argv)
{
    std::cout << (DESTINATIONS.count("NULL")) << std::endl;
    if(argc < 2)
    {
        std::cout << "Usage: hackc file..." << std::endl;
        return 1;
    }

    auto hack_file = std::string(argv[1]);
    auto hack_code = Load_Hack_Code(hack_file);
    Process_Labels(hack_code);

    try { 
        Compile_Hack_Code(hack_code); 
    } catch(const std::string& error) { std::cerr << error << '\n'; return 1; }


    std::ofstream binary_file(Split_String(hack_file,".")[0] + ".hack");
    for(auto& binray : BINARY)
        binary_file << binray << "\n";
    
    return 0;
}