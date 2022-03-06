import os
import psycopg2

conn = psycopg2.connect(
        host="localhost",
        database="hexa_server",
        user=os.environ['DB_USERNAME'],
        password=os.environ['DB_PASSWORD'])

# Open a cursor to perform database operations
cur = conn.cursor()

# Execute a command: this creates a new table
cur.execute('DROP TABLE IF EXISTS users;')
cur.execute('DROP TABLE IF EXISTS firmwares;')
cur.execute('DROP TABLE IF EXISTS targets;')
cur.execute('DROP TABLE IF EXISTS permissions;')

cur.execute('CREATE TABLE public.users'
            '(id integer NOT NULL,'
            'login character varying(50),'
            'password character varying(50));')

cur.execute('CREATE TABLE public.firmwares'
            '(id integer NOT NULL,'
            'nome character varying(20),'
            'version character varying(20),'
            'target integer);')

cur.execute('CREATE TABLE public.targets'
            '(id integer NOT NULL,'
            'name character varying(20));')

cur.execute('CREATE TABLE public.permissions'
            '(id_user integer,'
            'id_firm integer);')

cur.execute("INSERT INTO targets (id, name)"
            "VALUES (0, 'ESP32'), (1, 'ESP32S3'), (2, 'ESP8266');")

cur.execute('INSERT INTO users (id, login, password)'
            "VALUES (0, 'admin', 'admin');")

conn.commit()
cur.close()
conn.close()
