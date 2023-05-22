/*-*-c++-*-*************************************************************************************************************
* Copyright 2016 - 2022 Inesonic, LLC.
*
* This file is licensed under two licenses.
*
* Inesonic Commercial License, Version 1:
*   All rights reserved.  Inesonic, LLC retains all rights to this software, including the right to relicense the
*   software in source or binary formats under different terms.  Unauthorized use under the terms of this license is
*   strictly prohibited.
*
* GNU Public License, Version 2:
*   This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public
*   License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later
*   version.
*
*   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
*   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
*   details.
*
*   You should have received a copy of the GNU General Public License along with this program; if not, write to the Free
*   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
********************************************************************************************************************//**
* \file
*
* This file provides a trivial executable that can read an input file, compress it, and emit it as a C++ compatible
* declaration.
***********************************************************************************************************************/

#include <QByteArray>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ios>
#include <iomanip>
#include <algorithm>
#include <cstdio>

/**
 * Function that parses a single input file and dumps its contents.
 *
 * \param[in] inputStream        The input stream to be parsed.
 *
 * \param[in] outputStream       The name of the output file.  An empty string indicates stdout.
 *
 * \param[in] leftIndentation    Additional left side indentation.
 *
 * \param[in] indentation        The desired indentation in spaces.
 *
 * \param[in] width              The desired maximum line width.
 *
 * \param[in] prefix             An optional prefix in front of each variable name.
 *
 * \param[in] variableName       The payload variable name or suffix.
 *
 * \param[in] variableType       The variable type for the payload contents.
 *
 * \param[in] sizeVariableName   The size variable name or suffix.
 *
 * \param[in] sizeVariableType   The size variable type.
 *
 * \param[in] zlibCompress       Flag holding true if we should run the payload through the Qt compressor.
 *
 * \return Returns true on success.  Returns false on error.
 */
bool parseAndDumpInput(
        std::istream&      inputStream,
        std::ostream&      outputStream,
        unsigned           leftIndentation,
        unsigned           indentation,
        unsigned           width,
        const std::string& prefix,
        const std::string& variableName,
        const std::string& variableType,
        const std::string& sizeVariableName,
        const std::string& sizeVariableType,
        bool               zlibCompress
    ) {
    std::vector<unsigned char> inputBuffer;
    while (inputStream) {
        unsigned char readData;
        inputStream.read(reinterpret_cast<char*>(&readData), 1);
        inputBuffer.push_back(readData);
    }

    QByteArray byteArray(reinterpret_cast<const char*>(inputBuffer.data()), static_cast<int>(inputBuffer.size()));
    QByteArray outputData = zlibCompress ? qCompress(byteArray, 9) : byteArray;

    std::string leftIndentationString;
    for (unsigned column=0 ; column<leftIndentation ; ++column) {
        leftIndentationString += " ";
    }

    std::string contentsIndentationString;
    for (unsigned column=0 ; column<(leftIndentation + indentation) ; ++column) {
        contentsIndentationString += " ";
    }

    unsigned long numberBytes = outputData.size();

    outputStream << leftIndentationString << variableType << " "
                 << prefix << variableName << "[" << numberBytes << "] = {";

    unsigned valuesPerLine  = (width - indentation - leftIndentation + 1) / 6;
    unsigned valuesThisLine = valuesPerLine;
    for (unsigned long i=0 ; i<numberBytes ; ++i) {
        if (valuesThisLine >= valuesPerLine) {
            outputStream << std::endl << contentsIndentationString;
            valuesThisLine = 1;
        } else {
            ++valuesThisLine;
        }

        unsigned char v = outputData.at(i);

        char buffer[6];
        sprintf(buffer, "0x%02X", static_cast<unsigned>(v));
        outputStream << buffer;

        if (i < (numberBytes - 1)) {
            outputStream << ", ";
        }
    }

    outputStream << std::endl
                 << leftIndentationString << "};" << std::endl
                 << std::endl
                 << leftIndentationString << sizeVariableType << " " << prefix << sizeVariableName
                 << " = " << numberBytes << ";" << std::endl
                 << std::endl;

    return true;
}


/**
 * Function that performs the work of building a payload from one or more input files.
 *
 * \param[in] inputs             The list of input files.  An empty list indicates stdin.
 *
 * \param[in] outputStream       The stream to receive the generated output.
 *
 * \param[in] description        An optional description to place below the copyright message.
 *
 * \param[in] copyrightMessage   The copyright message to be included.
 *
 * \param[in] noCopyrightMessage A flag holding true if no copyright should be included.
 *
 * \param[in] indentation        The desired indentation in spaces.
 *
 * \param[in] width              The desired maximum line width.
 *
 * \param[in] namespaceName      An optional namespace to enacapsulate the payload in.
 *
 * \param[in] variableName       The payload variable name or suffix.
 *
 * \param[in] variableType       The variable type for the payload contents.
 *
 * \param[in] sizeVariableName   The size variable name or suffix.
 *
 * \param[in] sizeVariableType   The size variable type.
 *
 * \param[in] zlibCompress       Flag holding true if we should run the payload through the Qt compressor.
 *
 * \return Returns true on success.  Returns false on error.
 */
bool buildPayloadHelper(
        const std::vector<std::string>& inputs,
        std::ostream&                   outputStream,
        const std::string&              description,
        const std::string&              copyrightMessage,
        bool                            noCopyrightMessage,
        unsigned                        indentation,
        unsigned                        width,
        const std::string&              namespaceName,
        const std::string&              variableName,
        const std::string&              variableType,
        const std::string&              sizeVariableName,
        const std::string&              sizeVariableType,
        bool                            zlibCompress
    ) {
    bool success = true;

    if (!noCopyrightMessage || !description.empty()) {
        outputStream << "/*-*-c++-*-*";
        for (unsigned column=13 ; column<=width ; ++column) {
            outputStream << "*";
        }
        outputStream << std::endl;

        if (!noCopyrightMessage) {
            std::istringstream copyrightMessageStream(copyrightMessage);
            std::string        copyrightLine;
            while (std::getline(copyrightMessageStream, copyrightLine)) {
                outputStream << "* " << copyrightLine << std::endl;
            }
        }

        if (!noCopyrightMessage && !description.empty()) {
            for (unsigned column=1 ; column<=(width-4) ; ++column) {
                outputStream << "*";
            }

            outputStream << "//**" << std::endl;
        }

        if (!description.empty()) {
            outputStream << "* \\file" << std::endl
                         << "*" << std::endl;

            std::istringstream descriptionStream(description);
            std::string        descriptionLine;
            while (std::getline(descriptionStream, descriptionLine)) {
                outputStream << "* " << descriptionLine << std::endl;
            }
        }

        for (unsigned column=1 ; column<=(width-1) ; ++column) {
            outputStream << "*";
        }
        outputStream << "/" << std::endl
                     << std::endl;
    }

    unsigned leftIndentation = 0;
    if (!namespaceName.empty()) {
        outputStream << "namespace " << namespaceName << "{" << std::endl;
        leftIndentation = indentation;
    }

    if (inputs.empty()) {
        success = parseAndDumpInput(
            std::cin,
            outputStream,
            leftIndentation,
            indentation,
            width,
            "",
            variableName,
            variableType,
            sizeVariableName,
            sizeVariableType,
            zlibCompress
        );
    } else {
        if (inputs.size() == 1) {
            std::ifstream inputStream(inputs.at(0), std::ios::binary);

            if (inputStream) {
                success = parseAndDumpInput(
                    inputStream,
                    outputStream,
                    leftIndentation,
                    indentation,
                    width,
                    "",
                    variableName,
                    variableType,
                    sizeVariableName,
                    sizeVariableType,
                    zlibCompress
                );

                inputStream.close();
            } else {
                std::cerr << "*** Could not open input file " << inputs.at(0) << std::endl;
                success = false;
            }
        } else {
            std::vector<std::string>::const_iterator inputIterator    = inputs.cbegin();
            std::vector<std::string>::const_iterator inputEndIterator = inputs.cend();
            while (success && inputIterator != inputEndIterator) {
                std::string inputFilename = *inputIterator;
                std::ifstream inputStream(inputFilename, std::ios::binary);

                if (inputStream) {
                    std::size_t forwardSlashPosition = inputFilename.rfind('/');
                    std::size_t backslashPosition    = inputFilename.rfind('\\');

                    std::string prefix;
                    if (forwardSlashPosition != std::string::npos) {
                        if (backslashPosition != std::string::npos) {
                            std::size_t slashPosition = std::max(forwardSlashPosition, backslashPosition);
                            prefix = inputFilename.substr(slashPosition + 1);
                        } else {
                            prefix = inputFilename.substr(forwardSlashPosition + 1);
                        }
                    } else {
                        if (backslashPosition != std::string::npos) {
                            prefix = inputFilename.substr(backslashPosition + 1);
                        } else {
                            prefix = inputFilename;
                        }
                    }

                    std::replace(prefix.begin(), prefix.end(), '.', '_');

                    outputStream << "// Contents of " << inputFilename << ":" << std::endl;

                    success = parseAndDumpInput(
                        inputStream,
                        outputStream,
                        leftIndentation,
                        indentation,
                        width,
                        prefix,
                        variableName,
                        variableType,
                        sizeVariableName,
                        sizeVariableType,
                        zlibCompress
                    );

                    inputStream.close();
                    ++inputIterator;
                } else {
                    std::cerr << "*** Could not open input file " << inputs.at(0) << std::endl;
                    success = false;
                }
            }
        }
    }

    return success;
}


/**
 * Function that performs the work of building a payload from one or more input files.
 *
 * \param[in] inputs             The list of input files.  An empty list indicates stdin.
 *
 * \param[in] outputFilename     The name of the output file.  An empty string indicates stdout.
 *
 * \param[in] description        An optional description to place below the copyright message.
 *
 * \param[in] copyrightMessage   The copyright message to be included.
 *
 * \param[in] noCopyrightMessage A flag holding true if no copyright should be included.
 *
 * \param[in] indentation        The desired indentation in spaces.
 *
 * \param[in] width              The desired maximum line width.
 *
 * \param[in] namespaceName      An optional namespace to enacapsulate the payload in.
 *
 * \param[in] variableName       The payload variable name or suffix.
 *
 * \param[in] variableType       The variable type for the payload contents.
 *
 * \param[in] sizeVariableName   The size variable name or suffix.
 *
 * \param[in] sizeVariableType   The size variable type.
 *
 * \param[in] zlibCompress       Flag holding true if we should run the payload through the Qt compressor.
 *
 * \return Returns true on success.  Returns false on error.
 */
bool buildPayload(
        const std::vector<std::string>& inputs,
        const std::string&              outputFilename,
        const std::string&              description,
        const std::string&              copyrightMessage,
        bool                            noCopyrightMessage,
        unsigned                        indentation,
        unsigned                        width,
        const std::string&              namespaceName,
        const std::string&              variableName,
        const std::string&              variableType,
        const std::string&              sizeVariableName,
        const std::string&              sizeVariableType,
        bool                            zlibCompress
    ) {
    bool success;
    if (outputFilename.empty()) {
        success = buildPayloadHelper(
            inputs,
            std::cout,
            description,
            copyrightMessage,
            noCopyrightMessage,
            indentation,
            width,
            namespaceName,
            variableName,
            variableType,
            sizeVariableName,
            sizeVariableType,
            zlibCompress
        );
    } else {
        std::ofstream outputStream(outputFilename);

        if (outputStream) {
            success = buildPayloadHelper(
                inputs,
                outputStream,
                description,
                copyrightMessage,
                noCopyrightMessage,
                indentation,
                width,
                namespaceName,
                variableName,
                variableType,
                sizeVariableName,
                sizeVariableType,
                zlibCompress
            );

            outputStream.close();
        } else {
            std::cerr << "*** Could not open output file " << outputFilename << "." << std::endl;
            success = false;
        }
    }

    return success;
}


int main(int argumentCount, char* argumentValues[]) {
    bool                     success          = true;
    bool                     helpRequested    = false;
    std::string              description;
    std::string              outputFilename;
    std::string              copyrightMessage = "Copyright 2020 Inesonic, LLC.\nAll rights reserved.";
    bool                     removeCopyright  = false;
    unsigned                 indentation      = 4;
    unsigned                 width            = 120;
    std::string              namespaceName;
    std::string              variableName     = "declarations";
    std::string              variableType     = "static const unsigned char";
    std::string              sizeVariableName = "declarationsSize";
    std::string              sizeVariableType = "static const unsigned long";
    bool                     useZlib          = true;
    std::vector<std::string> inputs;

    unsigned argumentIndex = 1;
    while (success && !helpRequested && argumentIndex < static_cast<unsigned>(argumentCount)) {
        std::string argument(argumentValues[argumentIndex]);
        unsigned    remainingArguments = argumentCount - argumentIndex - 1;

        if (argument == "-h" || argument == "--help") {
            helpRequested = true;
        } else if (argument == "-o" || argument == "--output") {
            if (remainingArguments > 0) {
                ++argumentIndex;
                outputFilename = argumentValues[argumentIndex];
            } else {
                std::cerr << "*** The " << argument << " switch is missing a parameter." << std::endl;
                success = false;
            }
        } else if (argument == "-c" || argument == "--copyright") {
            if (remainingArguments > 0) {
                ++argumentIndex;
                copyrightMessage = argumentValues[argumentIndex];
            } else {
                std::cerr << "*** The " << argument << " switch is missing a parameter." << std::endl;
                success = false;
            }
        } else if (argument == "-C" || argument == "--no-copyright") {
            removeCopyright = true;
        } else if (argument == "-i" || argument == "--indentation") {
            if (remainingArguments > 0) {
                ++argumentIndex;
                indentation = strtoul(argumentValues[argumentIndex], nullptr, 10);
                if (indentation <= 0) {
                    std::cerr << "*** Invalid indentation value " << argumentValues[argumentIndex]  << std::endl;
                    success = false;
                }
            } else {
                std::cerr << "*** The " << argument << " switch is missing a parameter." << std::endl;
                success = false;
            }
        } else if (argument == "-w" || argument == "--width") {
            if (remainingArguments > 0) {
                ++argumentIndex;
                width = strtoul(argumentValues[argumentIndex], nullptr, 10);
                if (indentation <= 0) {
                    std::cerr << "*** Invalid width value " << argumentValues[argumentIndex]  << std::endl;
                    success = false;
                }
            } else {
                std::cerr << "*** The " << argument << " switch is missing a parameter." << std::endl;
                success = false;
            }
        } else if (argument == "-n" || argument == "--namespace") {
            if (remainingArguments > 0) {
                ++argumentIndex;
                namespaceName = argumentValues[argumentIndex];
            } else {
                std::cerr << "*** The " << argument << " switch is missing a parameter." << std::endl;
                success = false;
            }
        } else if (argument == "-v" || argument == "--variable") {
            if (remainingArguments > 0) {
                ++argumentIndex;
                variableName = argumentValues[argumentIndex];
            } else {
                std::cerr << "*** The " << argument << " switch is missing a parameter." << std::endl;
                success = false;
            }
        } else if (argument == "-t" || argument == "--type") {
            if (remainingArguments > 0) {
                ++argumentIndex;
                variableType = argumentValues[argumentIndex];
            } else {
                std::cerr << "*** The " << argument << " switch is missing a parameter." << std::endl;
                success = false;
            }
        } else if (argument == "-V" || argument == "--size-variable") {
            if (remainingArguments > 0) {
                ++argumentIndex;
                sizeVariableName = argumentValues[argumentIndex];
            } else {
                std::cerr << "*** The " << argument << " switch is missing a parameter." << std::endl;
                success = false;
            }
        } else if (argument == "-T" || argument == "--size-type") {
            if (remainingArguments > 0) {
                ++argumentIndex;
                sizeVariableType = argumentValues[argumentIndex];
            } else {
                std::cerr << "*** The " << argument << " switch is missing a parameter." << std::endl;
                success = false;
            }
        } else if (argument == "-z" || argument == "--zlib") {
            useZlib = true;
        } else if (argument == "-Z" || argument == "--no-zlib") {
            useZlib = false;
        } else {
            inputs.push_back(argument);
        }

        ++argumentIndex;
    }

    if (helpRequested) {
        std::cout << "Copyright 2020 Inesonic, LLC" << std::endl
                  << "This software is licensed under two terms:" << std::endl
                  << "  * The Inesonic Commercial License, Version 1" << std::endl
                  << "  * GNU Public License, Version 2" << std::endl
                  << std::endl
                  << "You can use this utility to convert a file, in raw binary form, to an C99 or" << std::endl
                  << "C++ array suitable for inclusion within a program.  You can use the program" << std::endl
                  << "to generate binary payloads of files." << std::endl
                  << std::endl
                  << "Command:" << std::endl
                  << "  build_payload [options] [ file [ file [ file ... ] ] ]" << std::endl
                  << std::endl
                  << "  -h | --help" << std::endl
                  << "    Display this help text, then exit.  All other switches are ignored." << std::endl
                  << std::endl
                  << "  --o <filename> | --output <filename>" << std::endl
                  << "    Specifies the name of the output file.  Output will be sent to stdout if" << std::endl
                  << "    this switch is not provided." << std::endl
                  << std::endl
                  << "  -c <message> | --copyright <message>" << std::endl
                  << "    Sets the displayed copyright message.  A standard copyright message will" << std::endl
                  << "    be included if this switch is not used." << std::endl
                  << std::endl
                  << "  -C | --no-copyright" << std::endl
                  << "    Removes all copyright messages from the output.  Using this switch will" << std::endl
                  << "    cause the -c/--copyright switch to be ignored." << std::endl
                  << std::endl
                  << "  -i <indentation> | --indentation <indentation>" << std::endl
                  << "    Specifies the indentation to use when formatting the output.  This tool" << std::endl
                  << "    will use an indentation of 4 spaces by default." << std::endl
                  << std::endl
                  << "  -w <width> | --width <width>" << std::endl
                  << "    Specifies the maximum line length.  The value is ignored when inserting" << std::endl
                  << "    the description and copyright messages." << std::endl
                  << std::endl
                  << "  -n <namespace> | --namespace <namespace>" << std::endl
                  << "    Specifies an optional namespace to place the generated content under." << std::endl
                  << std::endl
                  << "  -v <variable|suffix> | --variable <variable|suffix>" << std::endl
                  << "    Specifies the variable name to use for the payload.  If multiple input" << std::endl
                  << "    files are provided, then this switch specifies a suffix to append to" << std::endl
                  << "    the generated payloads.  Payload names will be based on the supplied" << std::endl
                  << "    filenames.  The default value is \"declarations\"" << std::endl
                  << std::endl
                  << "  -t <variable type> | --type <variable type>" << std::endl
                  << "    Specifies the type to assign to the payload array.  The default value" << std::endl
                  << "    is \"static const unsigned char\"." << std::endl
                  << std::endl
                  << "  -V <variable|suffix> | --size-variable <variable|suffix>" << std::endl
                  << "    Specifies the variable name to use for the payload size.  If multiple" << std::endl
                  << "    input files are provided, then this switch specifies a suffix to append" << std::endl
                  << "    to the generated payload sizes.  Payload names will be based on the " << std::endl
                  << "    supplied filenames.  The default value is \"declarationsSize\"" << std::endl
                  << std::endl
                  << "  -T <variable type> | --size-type <variable type<" << std::endl
                  << "    Specifies the type to assign to the payload size.  The default value" << std::endl
                  << "    is \"static const unsigned long\"." << std::endl
                  << std::endl
                  << "  -z | --zlib" << std::endl
                  << "    Indicates that the generated payload should be compressed using Qt's" << std::endl
                  << "    funky variant of the zlib compression algorithm.  This is the default" << std::endl
                  << "    behavior." << std::endl
                  << std::endl
                  << "  -Z | --no-zlib" << std::endl
                  << "    Indicates that the generated payload should not be compressed using" << std::endl
                  << "    Qt's variant of the zlib compression algorithm." << std::endl;
    } else if (success) {
        success = buildPayload(
            inputs,
            outputFilename,
            description,
            copyrightMessage,
            removeCopyright,
            indentation,
            width,
            namespaceName,
            variableName,
            variableType,
            sizeVariableName,
            sizeVariableType,
            useZlib
        );
    }

    return success ? 0 : 1;
}
