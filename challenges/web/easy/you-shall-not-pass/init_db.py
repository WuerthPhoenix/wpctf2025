import sqlite3

def init_db():
    with open('init.sql', 'r') as f:
        sql_script = f.read()
    
    conn = sqlite3.connect('database.db')
    cursor = conn.cursor()
    cursor.executescript(sql_script)
    conn.commit()
    conn.close()

if __name__ == '__main__':
    init_db()
