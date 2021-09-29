/*
* cpp_parse_config.h
*
* This is C++ one header file config file parser
* for work with simple key="value" per line styled
* config files.
* Parse config file and return std::unordered_map
* of paired std::string ("options"=>"value")
*
* See usage example at the end of file.
*
* Licensed under GNU General Public License v3
*
* Author: Kuzin Andrey <kuzinandrey@yandex.ru>
* (c) 2021
*/
#ifndef CPP_PARSE_CONFIG_H
#define CPP_PARSE_CONFIG_H

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

// some return error codes
#define CONFERR_NORET -1 // no valid pointer to return container
#define CONFERR_ERRFILE -2 // can't open config file
#define CONFERR_WRONGSYNTAX -3 // wrong syntax of config file
#define CONFERR_WRONGPARAM -4 // wrong parameter name
#define CONFERR_WRONGVALUE -5 // wrong parameter value

#ifndef CONF_PARAM_NAME_MAX_LEN
#define CONF_PARAM_NAME_MAX_LEN 30 // max length of parameter (buffer size)
#endif

#ifndef CONF_PARAM_VALUE_MAX_LEN
#define CONF_PARAM_VALUE_MAX_LEN 255 // max length of value (buffer size)
#endif

// Parse config file file_name and fill the unordered_map of strings "option"=>"value"
// return 0 on success or some error code
int parse_config(std::string file_name, std::unordered_map<std::string,std::string> *ret) {
	if (!ret) return CONFERR_NORET;

	std::ifstream fconf(file_name);
	if (!fconf) return CONFERR_ERRFILE;

	char param_name[CONF_PARAM_NAME_MAX_LEN];
	char param_value[CONF_PARAM_VALUE_MAX_LEN];
	int name_fill = 0;
	int value_fill = 0;

	char c;
	int line = 1;
	enum parse_mode {
		parse_skip_space
		, parse_skip_comment_line
		, parse_param_name
		, parse_skip_space_before_equal
		, parse_skip_space_after_equal
		, parse_value
		, parse_line_end
		, parse_value_in_single_quote
		, parse_value_in_double_quote
	};

	parse_mode mode = parse_skip_space;

	while (fconf.get(c)) {

		if (c == EOF) {
			if (mode == parse_value_in_single_quote || mode == parse_value_in_double_quote) {
				param_value[value_fill] = 0;
				ret->insert(std::make_pair(param_name,param_value));
			}
			break;
		}

		switch(mode) {

		case parse_skip_space:
			if (c == '#') { mode = parse_skip_comment_line; continue; }
			if (c == '\n') { line++; continue; }
			if (isalpha(c)) { // param name begin
				param_name[0] = c;
				name_fill = 1;
				mode = parse_param_name;
				continue;
			}
			if (!isspace(c)) {
				std::cerr << "Error in " << file_name
					<< ": param name can't start with not alpha char '"
					<< c << "' on line " << line << std::endl;
				fconf.close(); ret->clear();
				return CONFERR_WRONGPARAM;
			}
		break; // parse_skip_space

		case parse_skip_comment_line:
			if (c == '\n') {
				line++;
				mode = parse_skip_space;
			}
		break; // parse_skip_comment_line

		case parse_param_name:
			if (isspace(c)) { // name end
				if (c == '\n') line++;
				param_name[name_fill] = 0;
				name_fill++;
				mode = parse_skip_space_before_equal;
				continue;
			}
			if (c == '=') {
				param_name[name_fill] = 0;
				name_fill++;
				mode = parse_skip_space_after_equal;
				continue;
			}
			if (isalpha(c) || isdigit(c) || c == '_') {
				param_name[name_fill] = c;
				if (name_fill + 1 > CONF_PARAM_NAME_MAX_LEN - 1) {
					std::cerr << "Error in " << file_name
						<< ": param length is very big on " << line
						<< " line" << std::endl;
					fconf.close(); ret->clear();
					return CONFERR_WRONGPARAM;
				}
				name_fill++;
				continue;
			} else {
				std::cerr << "Error in " << file_name
					<< ": wrong char in param name '" << c << "' on "
					<< line << " line" << std::endl;
				fconf.close(); ret->clear();
				return CONFERR_WRONGPARAM;
			}
		break; // parse_param_name

		case parse_skip_space_before_equal:
			if (isspace(c)) {
				if (c == '\n') line++;
				continue;
			}
			if (c == '=') {
				mode = parse_skip_space_after_equal;
			}
		break; // parse_skip_space_before_equal

		case parse_skip_space_after_equal:
			if (isspace(c)) { if (c == '\n') line++; continue; }
			if (c == '\'') { mode = parse_value_in_single_quote; value_fill = 0; continue; }
			if (c == '"') { mode = parse_value_in_double_quote; value_fill = 0; continue; }
			if (c == '#') { // empty param value (comment line)
				value_fill = 0;
				param_value[value_fill] = 0;
				ret->insert(std::make_pair(param_name,param_value));
				mode = parse_skip_comment_line;
				continue;
			}
			param_value[0] = c;
			value_fill = 1;
			mode = parse_value;
		break; // parse_skip_space_after_equal

		case parse_value:
			if (isspace(c) || c == '#') {
				mode = parse_line_end;
				if (c == '#') mode = parse_skip_comment_line;
				param_value[value_fill] = 0;
				value_fill++;
				ret->insert(std::make_pair(param_name,param_value));
				if (c == '\n') { line++; mode = parse_skip_space; }
				continue;
			}
			param_value[value_fill] = c;
			if (value_fill + 1 > CONF_PARAM_VALUE_MAX_LEN - 1) {
				std::cerr << "Error in " << file_name << ": value length is very big on " << line << " line" << std::endl;
				fconf.close(); ret->clear();
				return CONFERR_WRONGVALUE;
			}
			value_fill++;
		break; // parse_value

		case parse_line_end:
			if (isspace(c)) {
				if (c == '\n') { line++; mode = parse_skip_space; }
				continue;
			}
			if (c == '#') {
				mode = parse_skip_comment_line;
				continue;
			}
			std::cerr << "Error in " << file_name
				<< ": wrong char '" << c
				<< "' on " << line << " line" << std::endl;
			fconf.close(); ret->clear();
			return CONFERR_WRONGSYNTAX;
		break; // parse_line_end

		case parse_value_in_single_quote:
			if (c != '\'' ){
				if (c == '\n') line++;
				param_value[value_fill] = c;
				if (value_fill + 1 > CONF_PARAM_VALUE_MAX_LEN - 1) {
					std::cerr << "Error in " << file_name
						<< ": value length is very big on"
						<< line << " line" << std::endl;
					fconf.close(); ret->clear();
					return CONFERR_WRONGVALUE;
				}
				value_fill++;
				continue;
			}
			param_value[value_fill] = 0;
			value_fill++;
			ret->insert(std::make_pair(param_name, param_value));
			mode = parse_skip_space;
		break; // parse_value_in_single_quote

		case parse_value_in_double_quote:
			if (c != '\"' ){
				if (c == '\n') line++;
				param_value[value_fill] = c;
				if (value_fill + 1 > CONF_PARAM_VALUE_MAX_LEN - 1) {
					std::cerr << "Error in " << file_name
						<< ": value length is very big on "
						<< line << " line" << std::endl;
					fconf.close(); ret->clear();
					return CONFERR_WRONGVALUE;
				}
				value_fill++;
				continue;
			}
			param_value[value_fill] = 0;
			value_fill++;
			ret->insert(std::make_pair(param_name, param_value));
			mode = parse_skip_space;
		break; // parse_value_in_double_quote

		} // switch
	} // while read conf

	return 0; // success
} // parse_config()


/*
#include <vector>
// Example of usage
int main() {

	std::string config_file_name  = "test.conf";

	// define variables for store options from config file
	// set default values for all options
	std::string mysql_host = "127.0.0.1";
	std::string mysql_user = "root";
	std::string mysql_password = "strongrootpassword";
	std::string mysql_database = "database";

	// make list of known options and variables for program
	struct config_options_st {
		std::string name;
		std::string *var_ptr;
	};

	std::vector<struct config_options_st> conf_params = {
		{"host", &mysql_host}
		,{"user", &mysql_user}
		,{"password", &mysql_password}
		,{"database", &mysql_database}
	};

	// result of parsing container
	std::unordered_map<std::string, std::string> conf;

	std::ofstream outconf(config_file_name);
	if (outconf) {
		outconf
		<< "# this is config file example" << std::endl
		<< "host=\"mysql.example.com\" # this is SQL host" << std::endl
		<< "user      =       'dba_admin'" << std::endl
		<< "password = helloworld # test comment" << std::endl
		<< "database=testdb123" << std::endl
		// << "unknown = 'test unknown value' # uncomment this line for test" << std::endl
		;
		outconf.close();
	} else {
		std::cerr << "Can't create config file " << config_file_name << std::endl;
		return -1;
	}


	// call parser
	if (parse_config(config_file_name, &conf) == 0) {
		for (auto c = conf.begin(); c != conf.end(); c++) {
			std::cout << "param=" << c->first << " value=" << c->second << std::endl;

			bool found = false;
			for (auto i = conf_params.begin(); i != conf_params.end(); i++) {
				if (i->name.compare(c->first) == 0) {
					*i->var_ptr = c->second;
					found = true; break;
				};
			}
			if (!found) {
				std::cerr << "Unknown parameter \"" << c->first << "\"" << std::endl;
				return -1;
			}
		}
	} else {
		std::cerr << "Can't parse config file " << config_file_name << std::endl;
		return -1;
	}

	conf.clear();

	std::cout << std::endl
		<< "MySQL config" << std::endl
		<< "- host: " << mysql_host << std::endl
		<< "- user: " << mysql_user << std::endl
		<< "- password: " << mysql_password << std::endl
		<< "- database: " << mysql_database << std::endl
	;

	return 0;
}
*/

#endif /* CPP_PARSE_CONFIG_H*/