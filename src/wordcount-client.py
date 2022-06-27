#!/usr/bin/env python3
###########################################################################
#                                                                         #
# Copyright 2022 INTERSEC SA                                              #
#                                                                         #
# Licensed under the Apache License, Version 2.0 (the "License");         #
# you may not use this file except in compliance with the License.        #
# You may obtain a copy of the License at                                 #
#                                                                         #
#     http://www.apache.org/licenses/LICENSE-2.0                          #
#                                                                         #
# Unless required by applicable law or agreed to in writing, software     #
# distributed under the License is distributed on an "AS IS" BASIS,       #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.#
# See the License for the specific language governing permissions and     #
# limitations under the License.                                          #
#                                                                         #
###########################################################################

import argparse

from pathlib import Path

# IOPy is a Python module to be able to easily manipulate the IOPs and call
# RPCs on other Intersec processes.
# It is available in lib-common/src/iopy.
import iopy


SCRIPT_DIR = Path(__file__).parent
PLUGIN_PATH = SCRIPT_DIR / "wordcount-plugin.so"


DESCRIPTION = """
Client part of wordcount.

Read the content of the file located at <file_path> and send it to the
server to get the number of occurrences of each unique words via RPC.

The configuration of the server is expected to be in IOP YAML as
described by the IOP `wordcount.ServerCfg`.
""".strip()


def main():
    # Parse the arguments
    parser = argparse.ArgumentParser(description=DESCRIPTION)
    parser.add_argument("-c", "--cfg",
                        help="path to the server configuration in YAML",
                        required=True)
    parser.add_argument("file_path",
                        help=(
                            "path to the file which content is sent to the "
                            "server"
                        ))
    args = parser.parse_args()

    # Load the plugin
    plugin = iopy.Plugin(str(PLUGIN_PATH))

    # Read the configuration
    cfg = plugin.wordcount.ServerCfg.from_file(_yaml=args.cfg)

    # Connect to the server
    ic = plugin.connect(f"{cfg.address}:{cfg.port}")

    # Read the file content
    with open(args.file_path, "r") as f:
        file_content = f.read()

    # Query the RPC with the file content
    res = ic.wordcount_Mod.wordcountIface.countOccurrences(
        fileContent=file_content)
    word_occurrences = res.wordOccurrences

    # Print the word occurrences
    for item in word_occurrences:
        print(f"{item.word} => {item.occurrences}")


if __name__ == '__main__':
    main()
