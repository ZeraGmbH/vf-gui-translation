import xml.etree.ElementTree as ET
import json, os, argparse

def tsToJson(tsFileNameWithPath, jsonFileNameWithPath):
    # Parse the XML file
    tree = ET.parse(tsFileNameWithPath)
    root = tree.getroot()
    translations = {}
    # Loop through the XML structure
    for context in root.findall('context'):
        for message in context.findall('message'):
            source = message.find('source').text
            translation = message.find('translation').text
            if translation is not None:
                translations[source] = translation
            else:
                translations[source] = source
    # Write to JSON
    with open(jsonFileNameWithPath, 'w', encoding='utf-8') as json_file:
        json.dump(translations, json_file, ensure_ascii=False, indent=4)
    print(f"Successfully converted {tsFileNameWithPath} to {jsonFileNameWithPath}")

def prepareJsonName(tsName):
    localeWithExt = tsName.split('_', 1)[1]
    return localeWithExt.replace('.ts', '.json')

def convertAllTsToJson(tsFiles, jsonDir):
    for tsFile in tsFiles:
        tsName = os.path.basename(tsFile)
        jsonFile = os.path.join(jsonDir, prepareJsonName(tsName))
        tsToJson(tsFile, jsonFile)

def parseArguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("tsFiles", type=str, help="A string with all .ts files separated by comma")
    parser.add_argument("outputDir", type=str, help="The directory for generated json files")
    args = parser.parse_args()
    # Split the string on commas and strip whitespace
    args.tsFiles = args.tsFiles.split(',')
    return args.tsFiles, args.outputDir

def main():
    tsFiles, jsonDir = parseArguments()
    convertAllTsToJson(tsFiles, jsonDir)

if __name__ == "__main__":
    main()
