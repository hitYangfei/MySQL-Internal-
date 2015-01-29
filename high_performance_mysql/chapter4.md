## Schema与数据类型优化

### 选择优化的数据类型

* 更小的通常更好：使用正确存储的最小数据类型
* 简单就好      ：使用内建的时间类型timestamp或者datetime存储时间而不用字符串，使用整数存储IP地址
* 尽量避免NULL

### 字符串 char ,varchar

* varchar

  varchar表示变长字符串，对空间的利空更好，在InnoDB或者Mysiam中。但是如果设置了ROW_FORMAT=FIXED则都是定长字符串，varchar不起作用。varchar使用一个字节或者两个字节表示字符串长度，假设采用latin1字符集，varchar(10)使用一个字节表示长度，varchar(1000)使用两个字节表示长度。varchar节省了磁盘空间，所以对性能有所帮助，但是由于行是变长的所以在更新时可能会导致行变得比原来要长，这就导致了一些额外的问题。MySQL5.0以后的版本会保留varchar类型末尾的空格。

  下面这些情况用varchar比较好：字符串的最大长度比平均长度大很多；列的更新不频繁所以碎片不是问题；使用了UTF-8这样的复杂编码，每个字符都使用不用的字节数进行存储。

* char
 
  char是固定长度的字符串，不会保留末尾的空格。char非常适合存储MD5编码因为长度固定

### BLOG TEXT

TEXT是用来存储长字符串类型的，BLOG是用来存储长二进制字符串的，前者需要编码如latin,UTF-8等，后者是二进制的字节码。MySQL对BLOG以及TEXT排序只会对前max_sort_length长度的字符进行排序

由于Memory存储引擎不支持BLOG和TEXT，如果在查询时使用了BLOG或者TEXT列并且用到了隐式的临时表，将不得不使用Mysiam临时标，这会导致严重的性能开销，即使配置了MySQL临时表在内存设备块上（RAM DISK）依然需要很多昂贵的系统调用。最好的办法的避免使用这两个字段，如果无法避免则进行前缀截取，使用substring函数将列转化为字符串，这样就可以使用内存临时表了，不过要保证截取的足够断，如果临时表大小超过tmp_table_size或者max_heap_table_size则会使用磁盘临时表。

可以使用枚举类型来替代字符串。

>CREATE TABLE enum_table (e ENUM('fish', 'apple', dog'') NOT NULL);

### DATETIME TIMESTAMP

* DATETIME

  范围从1001-9999,精度为妙，格式为YYYYMMDDHHMMSS与时区无关。使用八个字节的存储空间。

* TIMESTAMP

  保存从1970-01-01午夜12点（格林威治时间）以来的秒数。使用四个字节存储。时间范围从1970-2038.默认是当前时间且not null。

### BIT

Mysiam对于BIT(17)只需要三个字节进行存储，而Memory以及InnoDB对于每一个bit都会使用最小的类型进行存储，因此不会节省空间。

MySQL把BIT当作字符串类型而不是数字类型，结果是一个包含二进制0或者1的字符串。然后在数字上下文环境中当作数字来处理。如BIT(8)的00111001,检索它时得到的是字符\'9\'，是编码为57的字符。但是在数字的上下文中得到的是57。

>CREATE TABLE bittest(a bit(8));

>INSERT INTO bittest VALUES(b'00111001');

>SELECT a, a + 0 FROM bittest;

>得到的结果为9, 57
