import xml.etree.ElementTree as ET
import json, os, argparse

def ts_to_json(ts_file_with_path, json_file_with_path):
    # Parse the XML file
    tree = ET.parse(ts_file_with_path)
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
    with open(json_file_with_path, 'w', encoding='utf-8') as json_file:
        json.dump(translations, json_file, ensure_ascii=False, indent=4)
    print(f"Successfully converted {ts_file_with_path} to {json_file_with_path}")

def prepare_json_name(ts_name):
    locale_with_ext = ts_name.split('_', 1)[1]
    return locale_with_ext.replace('.ts', '.json')

def convert_all_ts_to_json(ts_files, json_dir):
    for ts_file in ts_files:
        ts_name = os.path.basename(ts_file)
        json_file = os.path.join(json_dir, prepare_json_name(ts_name))
        ts_to_json(ts_file, json_file)

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("ts_files", type=str, help="A string with all .ts files separated by comma")
    parser.add_argument("output_dir", type=str, help="The directory for generated json files")
    args = parser.parse_args()
    # Split the string on commas and strip whitespace
    args.ts_files = args.ts_files.split(',')
    return args.ts_files, args.output_dir

def main():
    ts_files, json_dir = parse_arguments()
    convert_all_ts_to_json(ts_files, json_dir)

if __name__ == "__main__":
    main()
