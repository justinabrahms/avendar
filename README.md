# Avendar

[![Circle CI](https://circleci.com/gh/justinabrahms/avendar.svg?style=svg)](https://circleci.com/gh/justinabrahms/avendar)
[![codecov.io](https://codecov.io/github/justinabrahms/avendar/coverage.svg?branch=master)](https://codecov.io/github/justinabrahms/avendar?branch=master)

## Getting it working.

We're not there yet. Here are the steps as I know them so far.


```
sudo aptitude install lib32gcc-4.8-dev # I don't think this is necessary?
sudo aptitude install libc6-dev:i386
sudo aptitude install libmysqlclient-dev:i386
sudo aptitude install g++-multilib  # Is this needed? Who knows!
sudo aptitude install libmysql{client,++}-dev:i386

```

mysql --cflags  things here looks promising: http://ubuntuforums.org/archive/index.php/t-1666018.html


Setup mysql.
```
mysql> create database pantheon;
Query OK, 1 row affected (0.00 sec)

mysql> create user pantheon identified by 'BuFF3L1ve5!';
Query OK, 0 rows affected (0.00 sec)

mysql> grant all on * to pantheon;
Query OK, 0 rows affected (0.00 sec)
```


## Status
### Factions
There's a bug wherein the last line of factions.txt is getting
interpreted as "" and it's not triggering the eof warning. Not sure
why this is. I have a small check in Faction::Read which states

```
        if (key == "")
            return true;
```

I think this is ultimately wrong and results in an extra faction than
should exist. It passes though (for now).


### Player lists

There seems to be an error getting player information. We're opening
`../player/_player.lst`, but it's empty. The setup doesn't expect
this.

NOTE: You can apparently just delete everything in the player/ dir, and it works.


### Help files

Player creation is buggy b/c helpfiles arne't loaded.

```
Mon Mar 28 02:17:16 2016 :: [*****] BUG: Query error 1146: Table 'pantheon.help_data' doesn't exist.
```

You can load the helpfiles in by starting a mysql session, selecting
the `pantheon` database and running `. help_db.sql` which will load
the help files. `help_db.sql` is a file in the initial avendar tarball
dump.
