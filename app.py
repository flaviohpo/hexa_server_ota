# usar logging para:
# - quem atualizou o firm
# - customizar status da resposta de erro dos get
# RESPONDER COM O HEADER DE ERRO! STATUS
# PRA ELE NAO QUERER DAR UPDATE COM A STRING DE EXCEPTION
# https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
# principais 200 400 500
# - responder e logar tbm o status
# fazer uma funcao que busca a versao dentro do arquivo pois
# este metodo é chamado em varios lugares 

from flask import Flask
from flask import send_file
from flask import render_template
import os.path
import re
import logging

logging.basicConfig(level = logging.INFO, 
                    filename = 'hexa_server.log', 
                    format = '%(asctime)s;%(levelname)s;%(message)s')        

logging.info('Hexa server started.')

app = Flask(__name__)

@app.route('/')
def home():
    logging.info('GET /')
    version_patern = re.compile(r'[0-9]+.[0-9]+.[0-9]+')
    with open('./espcode/version.h', 'r') as firm_version:
        b = firm_version.readline()
        while b:          
            match = version_patern.search(b)
            if match:
                version_patern = re.compile(r'[0-9]+')
                logging.info(f'return 200: version = {match.group(0)}')
                return render_template('header.html', versao=match.group(0))
            b = firm_version.readline()
    logging.error('return 400: bad request!')            
    return 'bad request!', 400

@app.route('/firmware')
def get_firmware():
    logging.info('GET /firmware')
    file_path = './espcode/build/espcode.bin'
    if not os.path.isfile(file_path):
        logging.error('return 400: failed to send file!')
        return 'file doesnt exists!', 400
    result = ''
    with open(file_path, 'rb') as firm:
        b = firm.read(1)
        while b:
            result += f'{bytes(b).hex()}'.upper()+' '
            b = firm.read(1)  
    logging.info('return 200: firmware in 0x ascii.')  
    return result

@app.route('/firmware_file')
def get_firmware_file():
    logging.info('GET /firmware_file')
    file_path = './espcode/build/espcode.bin'
    if not os.path.isfile(file_path):
        logging.error('return 400: file doesnt exists!')  
        return 'file doesnt exists!', 400
    try:
        logging.info(f'return 200: uploading {file_path}')  
        return send_file(file_path)
    except Exception as Ex:
        logging.error(f'return 400: failed to send file!')  
        return 'failed to send file!', 400

@app.route('/firmware_version')
def get_firmware_version():
    logging.info('GET /firmware_version')
    file_path = './espcode/version.h'
    if not os.path.isfile(file_path):
        logging.error('return 400: file doesnt exists!')  
        return 'file doesnt exists!', 400
    version_patern = re.compile(r'[0-9]+.[0-9]+.[0-9]+')
    with open(file_path, 'r') as firm_version:
        b = firm_version.readline()
        while b:          
            match = version_patern.search(b)
            if match:
                version_patern = re.compile(r'[0-9]+')
                logging.info(f'return 200: {match.group(0)}') 
                return(match.group(0))
            b = firm_version.readline()
    logging.error(f'return 400: bad request!') 
    return 'bad request!', 400

# start the server with the 'run()' method
if __name__ == '__main__':
    app.run(debug=True)