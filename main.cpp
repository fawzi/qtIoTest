#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QTextCodec>
#include <QDebug>

#include <functional>
#include "function_view.hpp"

void testWriter(QString label, QString path, std::function<void(QTextStream &)> writer, int repeatWrite = 1, int repeatFile = 1) {
    int flushFails = 0;
    qint64 writeTime = 0;
    qint64 writeFlushTime = 0;
    qint64 ioTime = 0;
    QElapsedTimer ioTimer;
    ioTimer.start();
    for (int iFile = 0; iFile < repeatFile; ++iFile) {
        QString newPath = path;
        if (iFile > 0) {
            QFileInfo fPath(path);
            newPath = fPath.dir().filePath(fPath.baseName() + QLatin1Char('-') + QString::number(iFile) + QLatin1Char('.') + fPath.completeSuffix());
        }
        QFile data(newPath);
        if (data.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&data);
            QElapsedTimer timer;
            timer.start();
            for (int iWrite = 0; iWrite < repeatWrite; ++iWrite)
                writer(out);
            writeTime += timer.nsecsElapsed();
            out.flush();
            writeFlushTime += timer.nsecsElapsed();
        } else {
            assert(false && "io error, could not open file");
        }
        if (!data.flush())
            ++flushFails;
    }
    ioTime = ioTimer.nsecsElapsed();
    qDebug() << label << "write:" << QString::number(double(writeTime)/1000000, 'f')
             << "writeFlush:" << QString::number(double(writeFlushTime)/1000000, 'f')
             << "ioTime:" << QString::number(double(ioTime)/1000000, 'f')
             << "flushFails:" << flushFails;
}


void writeQString(QTextStream &s, int repeat = 100) {
    s << QLatin1String("start\n");
    for (int i = 0; i < repeat; ++i) {
        QString base = QStringLiteral(u"bla bla");
        for (int j = 0; j < 3; ++j) {
            s << base.repeated(j);
            s << QStringLiteral(u"%1 bla %2 %3\n").arg(QStringLiteral(u"  ").repeated(j), base, base + base);
        }
    }
    s << QLatin1String("end\n");
}

using Sink = std::function<void(QStringView)>;
using SinkView = function_view<void(QStringView)>;

void repeatOut(Sink sink, int nrepeat, std::function<void(Sink)> dumper) {
    for (int i = 0; i < nrepeat; ++i)
        dumper(sink);
}

void repeatOutCRef(const Sink &sink, int nrepeat, const std::function<void(const Sink &)> &dumper) {
    for (int i = 0; i < nrepeat; ++i)
        dumper(sink);
}

void repeatOutView(SinkView sink, int nrepeat, function_view<void(SinkView)> dumper) {
    for (int i = 0; i < nrepeat; ++i)
        dumper(sink);
}

void writeDumper(QTextStream &s, int repeat = 100) {
    Sink sink = [&s](QStringView str){ s << str; };
    sink(u"start\n");
    for (int i = 0; i < repeat; ++i) {
        QStringView base = u"bla bla";
        for (int j = 0; j < 3; ++j) {
            repeatOut(sink, j, [base](Sink sink) { sink(base); });
            [base, j](Sink sink) {
                repeatOut(sink, j, [](Sink sink) { sink(u"  "); });
                sink(u" bla ");
                sink(base);
                sink(u" ");
                sink(base);
                sink(base);
                sink(u"\n");
            }(sink);
        }
    }
    sink(u"end\n");
}


void writeDumperCRef(QTextStream &s, int repeat = 100) {
    auto sink = [&s](QStringView str){ s << str; };
    sink(u"start\n");
    for (int i = 0; i < repeat; ++i) {
        QStringView base = u"bla bla";
        for (int j = 0; j < 3; ++j) {
            repeatOutCRef(sink, j, [base](const Sink &sink) { sink(base); });
            [base, j](const Sink &sink) {
                repeatOut(sink, j, [](Sink sink) { sink(u"  "); });
                sink(u" bla ");
                sink(base);
                sink(u" ");
                sink(base);
                sink(base);
                sink(u"\n");
            }(sink);
        }
    }
    sink(u"end\n");
}

void writeDumperView(QTextStream &s, int repeat = 100) {
    auto sink = [&s](QStringView str){ s << str; };
    sink(u"start\n");
    for (int i = 0; i < repeat; ++i) {
        QStringView base = u"bla bla";
        for (int j = 0; j < 3; ++j) {
            repeatOutView(sink, j, [base](SinkView sink) { sink(base); });
            [base, j](SinkView sink) {
                repeatOut(sink, j, [](SinkView sink) { sink(u"  "); });
                sink(u" bla ");
                sink(base);
                sink(u" ");
                sink(base);
                sink(base);
                sink(u"\n");
            }(sink);
        }
    }
    sink(u"end\n");
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    const int innerRepeat = 500;
    const int outerRepeat = 500;
    testWriter(QLatin1String("QStringAlloc"), QLatin1String("QStringAlloc.txt"),
               [](QTextStream &s) { writeQString(s, innerRepeat); }, outerRepeat, 1);
    testWriter(QLatin1String("LambdaAndSink"), QLatin1String("LambdaAndSink.txt"),
               [](QTextStream &s) { writeDumper(s, innerRepeat); }, outerRepeat, 1);
    testWriter(QLatin1String("LambdaAndSinkCRef"), QLatin1String("LambdaAndSinkCRef.txt"),
               [](QTextStream &s) { writeDumperCRef(s, innerRepeat); }, outerRepeat, 1);
    testWriter(QLatin1String("LambdaAndSinkView"), QLatin1String("LambdaAndSinkView.txt"),
               [](QTextStream &s) { writeDumperView(s, innerRepeat); }, outerRepeat, 1);
    return 0;
}
