#!/usr/bin/env python3

import unittest
import sys, os, shutil, json
import xml.etree.ElementTree as ET
sys.path.insert(0, '..')
from tsTojsonConverter import prepare_json_name, convert_all_ts_to_json, ts_to_json

#####################################################################
#                                                                   #
#   IMPORTANT NOTE                                                  #
# By Default these tests will not show up in Qt Creator             #
# Either enable them in project -> testing -> check CTest checkbox  #
# or run them locally by "./test_tsTojsonConverter.py"              #
#                                                                   #
#####################################################################

def get_test_files():
    return ["test-data/zera-gui_test_1.ts", "test-data/zera-gui_test_2.ts"]

def get_and_prepare_json_dir():
    global json_dir
    json_dir = sys.argv[1]
    if os.path.exists(json_dir):
        shutil.rmtree(json_dir) #cleanup the directory
    os.makedirs(json_dir)
    # unittest.main() parses sys.argv to discover which tests to run and with what verbosity.
    # Extra unknown arguments cause errors (unittest tries to interpret them as tests).
    # so delete argument before unittest.main()
    del sys.argv[1]

def count_xml_entries(xmlFile):
    tree = ET.parse(xmlFile)
    root = tree.getroot()
    message_count = 0
    for context in root.findall('context'):
        for message in context.findall('message'):
            message_count += 1
    return message_count

class Test_tsTojsonConverter(unittest.TestCase):

    def test_json_name(self):
        json_name = prepare_json_name(get_test_files()[0])
        self.assertEqual("test_1.json", json_name)

    def test_file_count(self):
        convert_all_ts_to_json(get_test_files(), json_dir)
        jsons_count = 0
        for entry in os.listdir(json_dir):
            if os.path.isfile(os.path.join(json_dir, entry)):
                jsons_count += 1
        self.assertEqual(len(get_test_files()), jsons_count)

    def test_json_content(self):
        dumped_json_file = os.path.join(json_dir, "test_1_dumped.json")
        ts_to_json(get_test_files()[0], dumped_json_file)
        xml_entries = count_xml_entries(get_test_files()[0])
        with open(dumped_json_file, 'r') as file:
            data = json.load(file)
            json_entries = len(data)
        self.assertEqual(xml_entries, json_entries)

if __name__ == "__main__":
    get_and_prepare_json_dir()
    unittest.main()
