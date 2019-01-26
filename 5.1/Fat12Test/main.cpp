#include <QtCore/QCoreApplication>
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QVector>
#include <QByteArray>

// 1字节对齐，没有内存空隙
#pragma pack(push)
#pragma pack(1)

struct Fat12Header
{
    char BS_OEMName[8];
    ushort BPB_BytsPerSec;
    uchar BPB_SecPerClus;
    ushort BPB_RsvdSecCnt;
    uchar BPB_NumFATs;
    ushort BPB_RootEntCnt;
    ushort BPB_TotSec16;
    uchar BPB_Media;
    ushort BPB_FATSz16;
    ushort BPB_SecPerTrk;
    ushort BPB_NumHeads;
    uint BPB_HiddSec;
    uint BPB_TotSec32;
    uchar BS_DrvNum;
    uchar BS_Reserved1;
    uchar BS_BootSig;
    uint BS_VolID;
    char BS_VolLab[11];
    char BS_FileSysType[8];
};

struct RootEntry
{
    char DIR_Name[11];       // 文件名8字节，拓展名3字节
    uchar DIR_Attr;          // 文件属性
    uchar reserve[10];       // 保留位
    ushort DIR_WrtTime;      // 最后一次写入时间
    ushort DIR_WrtDate;      // 最后一次写入日期
    ushort DIR_FstClus;      // 文件开始的簇号
    uint DIR_FileSize;       // 文件大小
};

#pragma pack(pop)

void PrintHeader(Fat12Header& rf, QString p)
{
    QFile file(p);

    if( file.open(QIODevice::ReadOnly) )
    {
        QDataStream in(&file);

        file.seek(3);

        in.readRawData(reinterpret_cast<char*>(&rf), sizeof(rf));

        rf.BS_OEMName[7] = 0;
        rf.BS_VolLab[10] = 0;
        rf.BS_FileSysType[7] = 0;

        qDebug() << "BS_OEMName: " << rf.BS_OEMName;
        qDebug() << "BPB_BytsPerSec: " << hex << rf.BPB_BytsPerSec;
        qDebug() << "BPB_SecPerClus: " << hex << rf.BPB_SecPerClus;
        qDebug() << "BPB_RsvdSecCnt: " << hex << rf.BPB_RsvdSecCnt;
        qDebug() << "BPB_NumFATs: " << hex << rf.BPB_NumFATs;
        qDebug() << "BPB_RootEntCnt: " << hex << rf.BPB_RootEntCnt;
        qDebug() << "BPB_TotSec16: " << hex << rf.BPB_TotSec16;
        qDebug() << "BPB_Media: " << hex << rf.BPB_Media;
        qDebug() << "BPB_FATSz16: " << hex << rf.BPB_FATSz16;
        qDebug() << "BPB_SecPerTrk: " << hex << rf.BPB_SecPerTrk;
        qDebug() << "BPB_NumHeads: " << hex << rf.BPB_NumHeads;
        qDebug() << "BPB_HiddSec: " << hex << rf.BPB_HiddSec;
        qDebug() << "BPB_TotSec32: " << hex << rf.BPB_TotSec32;
        qDebug() << "BS_DrvNum: " << hex << rf.BS_DrvNum;
        qDebug() << "BS_Reserved1: " << hex << rf.BS_Reserved1;
        qDebug() << "BS_BootSig: " << hex << rf.BS_BootSig;
        qDebug() << "BS_VolID: " << hex << rf.BS_VolID;
        qDebug() << "BS_VolLab: " << rf.BS_VolLab;
        qDebug() << "BS_FileSysType: " << rf.BS_FileSysType;

        file.seek(510);

        uchar b510 = 0;
        uchar b511 = 0;

        in.readRawData(reinterpret_cast<char*>(&b510), sizeof(b510));
        in.readRawData(reinterpret_cast<char*>(&b511), sizeof(b511));

        qDebug() << "Byte 510: " << hex << b510;
        qDebug() << "Byte 511: " << hex << b511;
    }

    file.close();
}

RootEntry FindRootEntry(Fat12Header& rf, QString p, int i)
{
    RootEntry ret = {{0}};

    QFile file(p);

    if( file.open(QIODevice::ReadOnly) && (0 <= i) && (i < rf.BPB_RootEntCnt) )
    {
        // 文件流
        QDataStream in(&file);

        // 文件目录项的位置在data.img中偏移19个扇区
        // 定位到要的第i个文件目录项
        file.seek(19 * rf.BPB_BytsPerSec + i * sizeof(RootEntry));

        // 将文件流中的内容读到文件目录项结构体中
        in.readRawData(reinterpret_cast<char*>(&ret), sizeof(ret));
    }

    file.close();

    // 返回这个结构体的内容
    return ret;
}

// 依据文件名读文件项
// fn文件名，p软盘路径
RootEntry FindRootEntry(Fat12Header& rf, QString p, QString fn)
{
    RootEntry ret = {{0}};

    for(int i=0; i<rf.BPB_RootEntCnt; i++)
    {
        RootEntry re = FindRootEntry(rf, p, i);

        if( re.DIR_Name[0] != '\0' )
        {
            // 传参中文件名里面点的位置
            int d = fn.lastIndexOf(".");
            // 文件目录项中的文件名
            QString name = QString(re.DIR_Name).trimmed();

            // 带点的文件名
            if( d >= 0 )
            {
                QString n = fn.mid(0, d);    // 文件名
                QString p = fn.mid(d + 1);   // 拓展名

                // 目录项中文件名前面语与n相等，后面与p相等
                if( name.startsWith(n) && name.endsWith(p) )
                {
                    // 找到要读的这个文件项，赋值返回
                    ret = re;
                    break;
                }
            }
            else   // 不带点的文件名
            {
                if( fn == name )
                {
                    ret = re;
                    break;
                }
            }
        }
    }

    return ret;
}

void PrintRootEntry(Fat12Header& rf, QString p)
{
    // BPB_RootEntCnt 最大跟目录文件数
    for(int i=0; i<rf.BPB_RootEntCnt; i++)
    {
        // 读第i个文件目录项
        RootEntry re = FindRootEntry(rf, p, i);

        // 依据名字判断这个文件目录项的合法性
        // 合法文件目录项输出相应的信息
        if( re.DIR_Name[0] != '\0' )
        {
            qDebug() << i << ":";
            qDebug() << "DIR_Name: " << hex << re.DIR_Name;
            qDebug() << "DIR_Attr: " << hex << re.DIR_Attr;
            qDebug() << "DIR_WrtDate: " << hex << re.DIR_WrtDate;
            qDebug() << "DIR_WrtTime: " << hex << re.DIR_WrtTime;
            qDebug() << "DIR_FstClus: " << hex << re.DIR_FstClus;
            qDebug() << "DIR_FileSize: " << hex << re.DIR_FileSize;
        }
    }
}

// Fat表：数据组织核心，记录文件数据的先后关系
// 读Fat表
QVector<ushort> ReadFat(Fat12Header& rf, QString p)
{
    QFile file(p);
    // BPB_BytsPerSec表示每扇区字节数，Fat表占9个扇区
    int size = rf.BPB_BytsPerSec * 9;
    uchar* fat = new uchar[size];
    // 每个fat表项用12比特，1.5字节，这里设计用2个字节的ushort容纳1个Fat表项足够
    // size * 2 / 3实际为 size / 1.5，为多少个Fat表项，这里初始化数组，每位初始化值为1
    QVector<ushort> ret(size * 2 / 3, 0xFFFF);

    // 打开Fat表
    if( file.open(QIODevice::ReadOnly) )
    {
        // 数据流
        QDataStream in(&file);

        // 定位
        file.seek(rf.BPB_BytsPerSec * 1);

        in.readRawData(reinterpret_cast<char*>(fat), size);

        // 每3个字节是两个Fat表项，i表示字节数，j表示Fat表项标号
        for(int i=0, j=0; i<size; i+=3, j+=2)
        {
            ret[j] = static_cast<ushort>((fat[i+1] & 0x0F) << 8) | fat[i];
            ret[j+1] = static_cast<ushort>(fat[i+2] << 4) | ((fat[i+1] >> 4) & 0x0F);
        }
    }

    file.close();

    delete[] fat;

    return ret;
}

// 输出文件名为fn的文件内容
// fn文件名，p软盘路径
QByteArray ReadFileContent(Fat12Header& rf, QString p, QString fn)
{
    QByteArray ret;
    RootEntry re = FindRootEntry(rf, p, fn);     // 读到了名为fn的文件目录项

    if( re.DIR_Name[0] != '\0' )
    {
        //
        QVector<ushort> vec = ReadFat(rf, p);
        QFile file(p);

        if( file.open(QIODevice::ReadOnly) )
        {
            // 一簇 512 字节
            char buf[512] = {0};
            QDataStream in(&file);
            int count = 0;

            // DIR_FileSize表示文件大小
            ret.resize(re.DIR_FileSize);

            // DIR_FstClus文件开始的簇号，Fat表项值为0xFF7说明文件损坏，Fat表项最后若大于0xFF8说明已经到达最后一个簇
            for(int i=0, j=re.DIR_FstClus; j<0xFF7; i+=512, j=vec[j])
            {
                // BPB_BytsPerSec扇区字节数，33文件数据区偏移扇区，Fat表前面有2个表项规定不使用所以文件数据区没有0和1直接从2标号开始
                // 将文件指针定位到Fat表项指示的簇
                file.seek(rf.BPB_BytsPerSec * (33 + j - 2));

                // 再从这个文件指针处开始读一簇的数到buf中
                in.readRawData(buf, sizeof(buf));

                // 将读到的数据放到ret中
                for(uint k=0; k<sizeof(buf); k++)
                {
                    // 读到的数据大小还小于文件大小，还没读完
                    if( count < ret.size() )
                    {
                        ret[i+k] = buf[k];
                        count++;
                    }
                }
            }
        }

        file.close();
    }

    return ret;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QString img = "../../data.img";
    Fat12Header f12;

    qDebug() << "Read Header:";

    // 输出FAT12文件系统的512Byte头（包含文件系统的重要信息和引导程序）
    PrintHeader(f12, img);

    qDebug() << endl;

    qDebug() << "Print Root Entry:";

    // 输出目录文件项
    PrintRootEntry(f12, img);

    qDebug() << endl;

    qDebug() << "Print File Content:";

    // 输出指定文件的内容 loader.bin
    QString content = QString(ReadFileContent(f12, img, "LOADER.BIN"));

    qDebug() << content;

    qDebug() << endl;
    // 输出指定文件的内容 写一个不存在的文件名
    content = QString(ReadFileContent(f12, img, "aaaa.BIN"));
    qDebug() << content;

    return a.exec();
}
/*运行结果：
Read Header:
BS_OEMName:  FreeDOS
BPB_BytsPerSec:  200
BPB_SecPerClus:  1
BPB_RsvdSecCnt:  1
BPB_NumFATs:  2
BPB_RootEntCnt:  e0
BPB_TotSec16:  b40
BPB_Media:  f0
BPB_FATSz16:  9
BPB_SecPerTrk:  12
BPB_NumHeads:  2
BPB_HiddSec:  0
BPB_TotSec32:  0
BS_DrvNum:  1
BS_Reserved1:  0
BS_BootSig:  29
BS_VolID:  0
BS_VolLab:
BS_FileSysType:  FAT12
Byte 510:  55
Byte 511:  aa


Print Root Entry:
0 :
DIR_Name:  At
DIR_Attr:  f
DIR_WrtDate:  ffff
DIR_WrtTime:  ffff
DIR_FstClus:  0
DIR_FileSize:  ffffffff
1 :
DIR_Name:  TEST    TXT
DIR_Attr:  20
DIR_WrtDate:  4e39
DIR_WrtTime:  7b7b
DIR_FstClus:  3
DIR_FileSize:  25
2 :
DIR_Name:  Al
DIR_Attr:  f
DIR_WrtDate:  0
DIR_WrtTime:  6e
DIR_FstClus:  0
DIR_FileSize:  ffffffff
3 :
DIR_Name:  LOADER  BIN
DIR_Attr:  20
DIR_WrtDate:  4e39
DIR_WrtTime:  7b7b
DIR_FstClus:  4
DIR_FileSize:  1ed


"Liyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\n\n"
"Liyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\nLiyao test!\n\n"


""
*/
