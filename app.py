# usar logging para:
# - quem atualizou o firm
# RESPONDER COM O HEADER DE ERRO! STATUS???
# PRA ELE NAO QUERER DAR UPDATE COM A STRING DE EXCEPTION
# https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
# principais 200 400 500
# - responder e logar tbm o status
# fazer uma funcao que busca a versao dentro do arquivo pois
# este metodo é chamado em varios lugares 

import urllib.request as req
import os.path
import re
import logging
import psycopg2
from datetime import datetime
from flask import Flask, send_file, render_template, redirect

logging.basicConfig(level = logging.INFO, 
                    filename = 'hexa_server.log', 
                    format = '%(asctime)s;%(levelname)s;%(message)s')  

logging.info('Hexa server started.')

############################
### CLASSES
class Firmware:
    def __init__(self, version, file_name, upload_date, target):
        self.version = version
        self.file_name = file_name
        self.upload_date = upload_date
        self.target = target

##################################
### HELPERS
def read_file_version():
    file_path = './espcode/version.h'
    if not os.path.isfile(file_path):
        logging.error('version file doesnt exists!')  
        return '0.0.0'
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

def get_db_connection():
    conn = psycopg2.connect(host='localhost',
                            database='hexa_server',
                            #user=os.environ['DB_USERNAME'],
                            #password=os.environ['DB_PASSWORD'])
                            user=os.environ['DB_USERNAME'],
                            password=os.environ['DB_PASSWORD'])
    return conn

###################################
### OBJECTS
version = read_file_version()
now = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
firm =  [   Firmware(version, 'espcode.bin', now, 'ESP32'),
            Firmware('1.1.0', 'BLCD.bin', now, 'ESP32S3'),   ]
app = Flask(__name__)

#################################
### ROUTES
@app.route('/')
def home():
    logging.info('GET /')
    return render_template('home.html', firmwares=firm)

@app.route('/login')
def login():
    logging.info('GET /login')
    return render_template('login.html');

@app.route('/autenticar', methods=['POST',])
def autenticar():
    logging.info('GET /autenticar')
    if req.form['user'] == 'admin' and req.form['pass'] == 'admin':
        return redirect('/')
    else:
        return redirect('/login')

@app.route('/users')
def get_users():
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute('SELECT * FROM users;')
    result = cur.fetchall()
    cur.close()
    conn.close()
    return result.__str__()

@app.route('/firmware')
def get_firmware():
    logging.info('GET /firmware')
    file_path = f'./espcode/build/{firm[0].file_name}'
    if not os.path.isfile(file_path):
        logging.error('return 400: failed to send file!')
        return 'file doesnt exists!', 400
    result = ''
    with open(file_path, 'rb') as fw:
        b = fw.read(1)
        while b:
            result += f'{bytes(b).hex()}'.upper()+' '
            b = fw.read(1)  
    logging.info('return 200: firmware in 0x ascii.')  
    return result

@app.route('/firmware_file')
def get_firmware_file():
    logging.info('GET /firmware_file')
    file_path = f'./espcode/build/{firm[0].file_name}'
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
    return(firm[0].version)

#################################################
### start the server with the 'run()' method
if __name__ == '__main__':
    app.run(debug=True)
