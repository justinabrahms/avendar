# Avendar

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
