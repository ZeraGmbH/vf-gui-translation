#!/usr/bin/env python3

import unittest
import sys, os, shutil
sys.path.insert(0, '..')
from tsTojsonConverter import convertAllTsToJson

#####################################################################
#                                                                   #
#   IMPORTANT NOTE                                                  #
# By Default these tests will not show up in Qt Creator             #
# Either enable them in project -> testing -> check CTest checkbox  #
# or run them locally by "./test_tsTojsonConverter.py"              #
#                                                                   #
#####################################################################

def get_testFiles():
    return ["test-data/zera-gui_test_1.ts", "test-data/zera-gui_test_2.ts"]

def getAndPrepareJsonDir():
    global jsonDir
    jsonDir = sys.argv[1]
    if os.path.exists(jsonDir):
        shutil.rmtree(jsonDir) #cleanup the directory
    os.makedirs(jsonDir)
    # unittest.main() parses sys.argv to discover which tests to run and with what verbosity.
    # Extra unknown arguments cause errors (unittest tries to interpret them as tests).
    # so delete argument before unittest.main()
    del sys.argv[1]

class Test_tsTojsonConverter(unittest.TestCase):

    def test_file_count(self):
        convertAllTsToJson(get_testFiles(), jsonDir)
        jsonsCount = 0
        for entry in os.listdir(jsonDir):
            if os.path.isfile(os.path.join(jsonDir, entry)):
                jsonsCount += 1
        self.assertEqual(len(get_testFiles()), jsonsCount)

if __name__ == "__main__":
    getAndPrepareJsonDir()
    unittest.main()
