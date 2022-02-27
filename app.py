# import the Flask class from the flask module
from flask import Flask
from flask import send_file
import re

# usar logging para:
# - quem atualizou o firm
# - customizar status da resposta de erro dos get
# RESPONDER COM O HEADER DE ERRO! STATUS
# PRA ELE NAO QUERER DAR UPDATE COM A STRING DE EXCEPTION
# https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
# principais 200 400 500
# - responder e logar tbm o status

# create the application object
app = Flask(__name__)

# use decorators to link the function to a url
@app.route('/')
def home():
    return "Hello, World!"  # return a string

@app.route('/firmware')
def get_firmware():
    result = ''
    with open('blink.bin', 'rb') as firm:
        b = firm.read(1)
        while b:
            result += f'{bytes(b).hex()}'.upper()+' '
            b = firm.read(1)    
    return result

#o ideal é tratar qual erro deu --> except Exception as ex:
@app.route('/firmware_file')
def get_firmware_file():
    try:
        return send_file('./espcode/build/espcode.bin')
    except Exception as Ex:
        return str(Ex)

# depois vai ter que mudar isso pra buscar a versao no version.h
@app.route('/firmware_version')
def get_firmware_version():
    version_patern = re.compile(r'[0-9]+.[0-9]+.[0-9]+')
    with open('./espcode/version.h', 'r') as firm_version:
        b = firm_version.readline()
        while b:          
            match = version_patern.search(b)
            if match:
                version_patern = re.compile(r'[0-9]+')
                print(match.group(0))
                return(match.group(0))
            b = firm_version.readline()
    return 'bad request!', 400

# start the server with the 'run()' method
if __name__ == '__main__':
    app.run(debug=True)