#include "mainwindow.h"

#include "./ui_mainwindow.h"

#include "cape_utils.h"

#include "config.h"

#include <QMessageBox>
#include <QDesktopServices>
#include <QSettings>
#include <QFileDialog>
#include <QTextStream>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QThread>
#include <QInputDialog>
#include <QCommandLineParser>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QOperatingSystemVersion>

#include "spdlog/spdlog.h"

#include "spdlog/sinks/qt_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include <filesystem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
	QCoreApplication::setApplicationName(PROJECT_NAME);
    QCoreApplication::setApplicationVersion(PROJECT_VER);
    ui->setupUi(this);

	auto const log_name{ "log.txt" };

	appdir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	std::filesystem::create_directory(appdir.toStdString());
	QString logdir = appdir + "/log/";
	std::filesystem::create_directory(logdir.toStdString());

	try
	{
		auto file{ std::string(logdir.toStdString() + log_name) };
		auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>( file, 1024 * 1024, 5, false);

		logger = std::make_shared<spdlog::logger>("esphomemanager", rotating);
		logger->flush_on(spdlog::level::debug);
		logger->set_level(spdlog::level::debug);
		logger->set_pattern("[%D %H:%M:%S] [%L] %v");
		spdlog::register_logger(logger);
	}
	catch (std::exception& /*ex*/)
	{
		QMessageBox::warning(this, "Logger Failed", "Logger Failed To Start.");
	}

	setWindowTitle(windowTitle() + " v" + PROJECT_VER);

	settings = std::make_unique< QSettings>(appdir + "/settings.ini", QSettings::IniFormat);

	RedrawRecentList();
	
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionClose_triggered()
{
	close();
}

void MainWindow::on_actionAbout_triggered()
{
	QString text = QString("ESPHome Manager v%1<br>QT v%2<br><br>Icons by:")
		.arg(PROJECT_VER, QT_VERSION_STR) +
		QStringLiteral("<br><a href='http://www.famfamfam.com/lab/icons/silk/'>www.famfamfam.com</a>");
		//http://www.famfamfam.com/lab/icons/silk/
	QMessageBox::about( this, "About ESPHome Manager Viewer", text );
}

void MainWindow::on_actionOpen_Logs_triggered()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(appdir + "/log/"));
}

void MainWindow::LoadEEPROM(QString const& filepath)
{
	settings->setValue("last_project", filepath);
	settings->sync();

	QFileInfo proj(filepath);
	m_cape = cape_utils::parseEEPROM(filepath.toStdString());

	ui->leProject->setText(m_cape.AsString().c_str());
	
	AddRecentList(proj.absoluteFilePath());

	ReadCapeInfo(m_cape.folder.c_str());
	CreateStringsList(m_cape.folder.c_str());
	ReadGPIOFile(m_cape.folder.c_str());
	ReadOtherFile(m_cape.folder.c_str());
}

void MainWindow::ReadCapeInfo(QString const& folder)
{
	//ui->textEditCapeInfo->clear();
	QFile infoFile(folder + "/cape-info.json");
	if (!infoFile.exists())
	{
		LogMessage("cape-info file not found", spdlog::level::level_enum::err);
		return;
	}

	if (!infoFile.open(QIODevice::ReadOnly))
	{
		LogMessage("Error Opening: cape-info.json" , spdlog::level::level_enum::err);
		return;
	}

	QByteArray saveData = infoFile.readAll();
	//ui->textEditCapeInfo->setText(saveData);
}

void MainWindow::CreateStringsList(QString const& folder)
{
	QDir directory(folder + "/strings");
	auto const& stringFiles = directory.entryInfoList(QStringList() << "*.json" , QDir::Files);

	//ui->comboBoxCape->clear();
	
	for (auto const& file : stringFiles)
	{
		//ui->comboBoxCape->addItem(file.fileName());
	}
}


void MainWindow::LogMessage(QString const& message, spdlog::level::level_enum llvl)
{
	logger->log(llvl, message.toStdString());
}


