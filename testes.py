with open('blink.bin', 'rb') as firm:
    b = firm.read(1)
    while b:
        print(f'0x'+f'{bytes(b).hex()}'.upper(), end=' ')
        b = firm.read(1)
