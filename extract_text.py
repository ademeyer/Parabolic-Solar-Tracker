import zipfile
import xml.etree.ElementTree as ET

def extract_text_from_docx(docx_path):
    with zipfile.ZipFile(docx_path, 'r') as docx:
        xml_content = docx.read('word/document.xml')
    root = ET.fromstring(xml_content)
    ns = {'w': 'http://schemas.openxmlformats.org/wordprocessingml/2006/main'}
    texts = []
    for elem in root.iter():
        if elem.tag == '{http://schemas.openxmlformats.org/wordprocessingml/2006/main}t':
            texts.append(elem.text or '')
    return '\n'.join(texts)

print(extract_text_from_docx('/home/laplace/Downloads/Full PhD Proposal Thesis updated.docx'))