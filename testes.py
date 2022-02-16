with open('blink.bin', 'rb') as firm:
    b = firm.read(1)
    while b:
        print(f'0x'+f'{bytes(b).hex()}'.upper(), end=' ')
        b = firm.read(1)



# inicio do trecho python
lista_apostilas = ['HTML, CSS e Javascript', 'Java para Web']

@app.route('/apostilas')
def apostilas_online():
    return render_template('apostilas.html', lista=lista_apostilas)

# fim trecho