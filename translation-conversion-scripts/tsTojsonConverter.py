import xml.etree.ElementTree as ET
import json

def ts_to_json(ts_file_path, json_file_path):
    # Parse the XML file
    tree = ET.parse(ts_file_path)
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
    with open(json_file_path, 'w', encoding='utf-8') as json_file:
        json.dump(translations, json_file, ensure_ascii=False, indent=4)

    print(f"Successfully converted {ts_file_path} to {json_file_path}")

ts_to_json('zera-gui_de_DE.ts', 'de_DE.json')
ts_to_json('zera-gui_en_GB.ts', 'en_GB.json')