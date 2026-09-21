#include "plugin/engine.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
int main(int argc,char **argv) {
 QCoreApplication app(argc,argv); QTemporaryDir dir; comments::Engine engine;
 QObject::connect(&engine,&comments::Engine::youtubeStatus,&app,[&](const QString &status){std::cout<<status.toStdString()<<std::endl;app.quit();});
 QTimer::singleShot(0,&app,[&]{engine.start({"dY4gYgNme4M","oauth:"+QString::fromLocal8Bit(argv[1]),"unused",dir.path()});});
 QTimer::singleShot(20000,&app,&QCoreApplication::quit);
 app.exec();engine.stop();
}
