## MySQL 认证过程

client:172.16.70.36

server:172.16.90.12

mysql -uroot -h127.0.0.1 -P13001

1. server-&gt;client : ShakeHandPacket

>4e:00:00:00:0a:35:2e:36:2e:31:34:2d:6c:6f:67:00

>10:0d:00:00:5b:73:4f:72:71:5e:65:61:00:ff:f7:08

>02:00:7f:80:15:00:00:00:00:00:00:00:00:00:00:38

>2f:25:6a:68:4c:22:62:48:24:78:50:00:6d:79:73:71

>6c:5f:6e:61:74:69:76:65:5f:70:61:73:73:77:6f:72

>64:00

>length                     : 82

>packetid                   : 0

>protocol version           : 0a

>server version             : 5.6.14-log

>connection id              : 0xd10

>auth-plguin-data-par1      : [sOrq^ea

>filter                     : 0

>cap flags lower 2 byte     : ff:f7

>charset                    : 08

>status flags               : 02:00

>cap flags hith 2 byte      : 7f:80                capablity=0x807ff7ff

>CLIENT_PLUGIN_AUTH         : length 0x15=21

>reversed 10 0x0

>atuh-plugin-data-per2      : 8/%jhL"bH$xP

>auth-plugin-name           : mysql_native_password


2. client-&gt;server : AuthPacket

>ac:00:00:01:85:a6:7f:00:00:00:00:01:21:00:00:00

>00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00

>00:00:00:00:72:6f:6f:74:00:00:6d:79:73:71:6c:5f

>6e:61:74:69:76:65:5f:70:61:73:73:77:6f:72:64:00

>6f:03:5f:6f:73:0e:6c:69:6e:75:78:2d:67:6c:69:62

>63:32:2e:35:0c:5f:63:6c:69:65:6e:74:5f:6e:61:6d

>65:08:6c:69:62:6d:79:73:71:6c:04:5f:70:69:64:05

>31:33:36:37:34:0f:5f:63:6c:69:65:6e:74:5f:76:65

>72:73:69:6f:6e:06:35:2e:36:2e:31:32:09:5f:70:6c

>61:74:66:6f:72:6d:06:78:38:36:5f:36:34:0c:70:72

>6f:67:72:61:6d:5f:6e:61:6d:65:05:6d:79:73:71:6c


>length                     : 176

>packetid                   : 1

>capablitys                 : 0x007fa685

>max_packet                 : 0x01000000

>chrset                     : 0x21

>reserved 23 0x0 

>username                   : root

>length of auth-reponse     : 0

>auth-plugin-name           : mysql_native_password

>length of k-v              : 6f = 96 + 15 = 11

>length-key 3 key _os   length-value 14  value linux-glibc2.5

>length-key 12 key _client_name length-value 8 value libmysql

>length-key 04 key _pid                      5 13674

>                  _client_version             5.6.12
>
>                  platform                    x86_64
>
>                  program_name                mysql
>

3. server-&gt;client :AuthResponse

>07:00:00:02:00:00:00:02:00:00:00

>length                     : 7

>packetid                   : 2

>OK packet                  : 00

>affected_rows              : 00

>last_insert_id             : 00

>server_status              : 0x2

>warnings                   : 0



