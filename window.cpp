/****************************************************************************
**
** Copyright (C) 2021 Anders F Björklund
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "window.h"

#ifndef QT_NO_SYSTEMTRAYICON

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QProcess>
#include <QThread>
#include <QDebug>

#ifndef QT_NO_TERMWIDGET
#include <QApplication>
#include <QMainWindow>
#include <QStandardPaths>
#include "qtermwidget.h"
#endif

//! [0]
Window::Window()
{
    createStatusGroupBox();
    iconComboBox = new QComboBox;
    iconComboBox->addItem(QIcon(":/images/minikube.png"), tr("minikube"));

    createActions();
    createTrayIcon();

    connect(sshButton, &QAbstractButton::clicked, this, &Window::sshConsole);
    connect(updateButton, &QAbstractButton::clicked, this, &Window::updateStatus);
    connect(startButton, &QAbstractButton::clicked, this, &Window::startMachine);
    connect(stopButton, &QAbstractButton::clicked, this, &Window::stopMachine);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &Window::iconActivated);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusGroupBox);
    setLayout(mainLayout);

    setIcon(0);
    trayIcon->show();

    setWindowTitle(tr("minikube"));
    setWindowIcon(*trayIconIcon);
    resize(400, 300);
}
//! [0]

//! [1]
void Window::setVisible(bool visible)
{
    minimizeAction->setEnabled(visible);
    maximizeAction->setEnabled(!isMaximized());
    restoreAction->setEnabled(isMaximized() || !visible);
    QDialog::setVisible(visible);
}
//! [1]

//! [2]
void Window::closeEvent(QCloseEvent *event)
{
#ifdef Q_OS_OSX
    if (!event->spontaneous() || !isVisible()) {
        return;
    }
#endif
    if (trayIcon->isVisible()) {
        QMessageBox::information(this, tr("Systray"),
                                 tr("The program will keep running in the "
                                    "system tray. To terminate the program, "
                                    "choose <b>Quit</b> in the context menu "
                                    "of the system tray entry."));
        hide();
        event->ignore();
    }
}
//! [2]

//! [3]
void Window::setIcon(int index)
{
    QIcon icon = iconComboBox->itemIcon(index);
    trayIcon->setIcon(icon);

    trayIcon->setToolTip(iconComboBox->itemText(index));
}
//! [3]

//! [4]
void Window::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        iconComboBox->setCurrentIndex((iconComboBox->currentIndex() + 1) % iconComboBox->count());
        break;
    default:
        ;
    }
}
//! [4]

void Window::createStatusGroupBox()
{
    statusGroupBox = new QGroupBox(tr("Status"));

    QIcon updateIcon = QIcon(":/images/view-refresh.png");
    updateButton = new QPushButton(updateIcon, "");
    updateButton->setFixedWidth(32);
    nameLabel = new QLabel("");
    sshButton = new QPushButton(tr("SSH"));
    statusLabel = new QLabel("Unknown");

#ifdef QT_NO_TERMWIDGET
    sshButton->setEnabled(false);
#endif

    startButton = new QPushButton(tr("Start"));
    stopButton = new QPushButton(tr("Stop"));

    updateStatus();

    QHBoxLayout *statusLayout = new QHBoxLayout;
    statusLayout->addWidget(updateButton);
    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(sshButton);
    statusLayout->addWidget(startButton);
    statusLayout->addWidget(stopButton);
    statusGroupBox->setLayout(statusLayout);
}

void Window::sshConsole()
{
#ifndef QT_NO_TERMWIDGET
    QMainWindow *mainWindow = new QMainWindow();
    int startnow = 0; // set shell program first

    QTermWidget *console = new QTermWidget(startnow);

    QFont font = QApplication::font();
    font.setFamily("Monospace");
    font.setPointSize(10);

    console->setTerminalFont(font);
    console->setColorScheme("Tango");
    console->setShellProgram(QStandardPaths::findExecutable("minikube"));
    QStringList args = {"ssh"};
    console->setArgs(args);
    console->startShellProgram();

    QObject::connect(console, SIGNAL(finished()), mainWindow, SLOT(close()));

    mainWindow->setWindowTitle(nameLabel->text());
    mainWindow->resize(800, 400);
    mainWindow->setCentralWidget(console);
    mainWindow->show();
#endif
}

bool Window::getProcessOutput(QStringList arguments, QString& text) {
    bool success;

    QString program = "minikube";

    QProcess *process = new QProcess(this);
    process->start(program, arguments);
    success = process->waitForFinished();
    if (success) {
        text = process->readAllStandardOutput();
    }
    delete process;
    return success;
}

void Window::updateStatus()
{
    QStringList arguments;
    arguments << "status" << "--format" << "{{.Host}}";

    QString *text = new QString();
    bool success = getProcessOutput(arguments, *text);
    if (success) {
        if (text->startsWith("Running")) {
            statusLabel->setText(tr("Running"));
            startButton->setEnabled(false);
            stopButton->setEnabled(true);
        } else if (text->startsWith("Stopped")) {
            statusLabel->setText(tr("Stopped"));
            startButton->setEnabled(true);
            stopButton->setEnabled(false);
        } else {
            statusLabel->setText(tr("Not Started "));
            startButton->setEnabled(false);
            stopButton->setEnabled(false);
        }
    }
    delete text;
}

void Window::sendMachineCommand(QString cmd)
{
    QString program = "minikube";
    QStringList arguments;
    arguments << cmd;
    bool success;

    QProcess *process = new QProcess(this);
    process->start(program, arguments);
    this->setCursor(Qt::WaitCursor);
    success = process->waitForFinished();
    this->unsetCursor();

    if (!success) {
        qDebug() << process->readAllStandardOutput();
        qDebug() << process->readAllStandardError();
    }
}

void Window::startMachine()
{
    sendMachineCommand(QString("start"));
    updateStatus();
}

void Window::stopMachine()
{
    sendMachineCommand(QString("stop"));
    updateStatus();
}

void Window::createActions()
{
    minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(minimizeAction, &QAction::triggered, this, &QWidget::hide);

    maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void Window::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIconIcon = new QIcon(":/images/minikube.png");

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(*trayIconIcon);
}

#endif
