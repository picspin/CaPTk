#pragma once

#include <string>
#include <QString>
#include <CaPTkDefines.h>
#include <QMessageBox>
#include <QFileDialog>
#include <QComboBox>
#include <QCoreApplication>

static QString IMAGES_EXTENSIONS = "Images (*.nii.gz *.nii *.dcm)";

inline void fixComboBox(QComboBox *comboBoxToEdit)
{
  for (int i = 0; i < comboBoxToEdit->count(); i++)
  {
    comboBoxToEdit->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);
  }
}

/*
\param message The message to be displayed in the message box
\param windowTitle The title of the message box, defaults to "Error"
*/

inline void ShowErrorMessage(const std::string &message, QWidget *boxParent = NULL, const std::string &windowTitle = "Error")
  {
    if (boxParent == NULL)
    {
      QMessageBox *box = new QMessageBox();
      box->setIcon(QMessageBox::Information);
      box->addButton(QMessageBox::Ok);
      box->setText(QString(message.c_str()));
      box->setWindowTitle(QString(windowTitle.c_str()));
      box->setAttribute(Qt::WA_DeleteOnClose); //makes sure the msgbox is deleted automatically when closed
      box->exec();
    }
    else
    {
      QMessageBox *box = new QMessageBox(boxParent);
      box->setIcon(QMessageBox::Information);
      box->addButton(QMessageBox::Ok);
      box->setText(QString(message.c_str()));
      box->setWindowTitle(QString(windowTitle.c_str()));
      box->setAttribute(Qt::WA_DeleteOnClose); //makes sure the msgbox is deleted automatically when closed
      box->exec();
    }
  }

  /**
  \brief Displays the message in a small message box with default window title as "Output Results"

  This function is used for displaying regular output results which aren't errors.

  For error, use ShowErrorMessage() function. For simple success notification, use the updateProgress(), function.

  \param message The message to be displayed in the message box
  \param windowTitle The title of the message box, defaults to "Output Results"
  */
inline void ShowMessage(const std::string &message, const std::string &windowTitle = "Output Results", QWidget *boxParent = NULL)
  {
    if (boxParent == NULL)
    {
      QMessageBox *box = new QMessageBox();
      box->setIcon(QMessageBox::Information);
      box->setStandardButtons(QMessageBox::Ok);
      box->setText(QString(message.c_str()));
      box->setWindowTitle(QString(windowTitle.c_str()));
      box->setAttribute(Qt::WA_DeleteOnClose); //makes sure the msgbox is deleted automatically when closed
      //box->setModal(false); // if you want it non-modal
      box->setWindowModality(Qt::NonModal);
      QCoreApplication::processEvents();
      box->show();
      QCoreApplication::processEvents();
      //box->open(this, SLOT(msgBoxClosed(QAbstractButton*)));
    }
    else
    {
      QMessageBox *box = new QMessageBox(boxParent);
      box->setIcon(QMessageBox::Information);
      box->setStandardButtons(QMessageBox::Ok);
      box->setText(QString(message.c_str()));
      box->setWindowTitle(QString(windowTitle.c_str()));
      box->setAttribute(Qt::WA_DeleteOnClose); //makes sure the msgbox is deleted automatically when closed
      //box->setModal(false); // if you want it non-modal
      box->setWindowModality(Qt::NonModal);
      QCoreApplication::processEvents();
      box->show();
      QCoreApplication::processEvents();
      //box->open(this, SLOT(msgBoxClosed(QAbstractButton*)));
    }
  }

  /**
  \brief Open existing directory (this is the mechanism to use for saving directories as well)

  \param parent The QWidget pointer to inherit from
  \param inputPath The path on which to start the navigation
  */
inline QString getExistingDirectory(QWidget *parent, const QString inputPath)
  {
    QFileDialog fileDialog;
    fileDialog.setWindowFlags(fileDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QString directory = fileDialog.getExistingDirectory(parent, "Open Directory", inputPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);

    if (!directory.isNull())
    {
      return directory;
    }
    else
    {
      ShowErrorMessage("No folder selected; please try again");
      return "";
    }
  }
  /**
  \brief Open existing directory (this is the mechanism to use for saving directories as well)

  \param parent The QWidget pointer to inherit from
  \param inputPath The path on which to start the navigation
  */
  inline QString getExistingDirectory(QWidget *parent, const std::string inputPath)
  {
    return getExistingDirectory(parent, QString(inputPath.c_str()));
  }

  /**
  \brief Open existing file

  \param parent The QWidget pointer to inherit from
  \param inputPath The path on which to start the navigation
  \param extensions The File type and corresponding extensions as shown in the exemplary default case
  */
  inline QString getExistingFile(QWidget *parent, const QString inputPath, const QString extensions = IMAGES_EXTENSIONS)
  {
    QFileDialog fileDialog;
    fileDialog.setWindowFlags(fileDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QString filename = fileDialog.getOpenFileName(parent, "Select File", inputPath, extensions, 0, QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);

    if (!filename.isNull())
    {
      return filename;
    }
    else
    {
      ShowErrorMessage("No file selected; please try again");
      return "";
    }
  }


  /**
  \brief Open existing file

  \param parent The QWidget pointer to inherit from
  \param inputPath The path on which to start the navigation
  \param extensions The File type and corresponding extensions as shown in the exemplary default case
  */
  inline QString getExistingFile(QWidget *parent, const std::string inputPath, const std::string extensions =IMAGES_EXTENSIONS.toStdString())
  {
    return getExistingFile(parent, QString(inputPath.c_str()), QString(extensions.c_str()));
  }

  /**
  \brief Get file name to save (doesn't have to be existing)

  \param parent The QWidget pointer to inherit from
  \param inputPath The path on which to start the navigation
  \param extensions The File type and corresponding extensions as shown in the exemplary default case
  \param defaultFileName The file name that is populated by default
  */
 inline QString getSaveFile(QWidget *parent, const QString inputPath, const QString defaultFileName = "", const QString extensions = IMAGES_EXTENSIONS)
  {
  QFileDialog fileDialog(parent, "Save File", inputPath, extensions);
  fileDialog.setWindowFlags(fileDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
  fileDialog.setOptions(QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);
  fileDialog.selectFile(defaultFileName);
  fileDialog.setFileMode(QFileDialog::AnyFile);
  fileDialog.setAcceptMode(QFileDialog::AcceptSave);

  //QString filename = fileDialog.getSaveFileName(parent, "Save File", inputPath.c_str(), extensions.c_str(), 0, QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);
  int ret = fileDialog.exec();
  if (ret == QDialog::Accepted)
  {
    return fileDialog.selectedFiles()[0];
  }
  else
  {
    //ShowErrorMessage("No file selected; please try again");
    return "";
  }
}
  /**
  \brief Get file name to save (doesn't have to be existing)

  \param parent The QWidget pointer to inherit from
  \param inputPath The path on which to start the navigation
  \param extensions The File type and corresponding extensions as shown in the exemplary default case
  \param defaultFileName The file name that is populated by default
  */
  inline QString getSaveFile(QWidget *parent, const std::string inputPath, const std::string defaultFileName = "", const std::string extensions = IMAGES_EXTENSIONS.toStdString())
  {
    return getSaveFile(parent, QString(inputPath.c_str()), QString(defaultFileName.c_str()), QString(extensions.c_str()));
  }


  /**
  \brief Get full path of the executable from the application name
  */
  inline std::string getApplicationPath(std::string appName)
  {
    std::string winExt
#if WIN32
      = ".exe"
#endif
      ;

    auto appName_wrap = appName;

    if (appName_wrap.find("libra") != std::string::npos)
    {
#if WIN32
      winExt = ".bat";
#elif linux
      winExt = "";
#endif
    }
    else if (appName_wrap.find("confetti") != std::string::npos)
    {
      appName_wrap = "ConfettiGUI";
#ifndef _WIN32
      winExt = ".py";
#endif
    }
    else if (appName_wrap.find("Theia") != std::string::npos)
    {
#ifndef _WIN32
      winExt = ".py";
#endif
    }
    else if (appName_wrap.find("DeepMedic") != std::string::npos)
    {
      appName_wrap = "deepMedicRun";
#ifndef _WIN32
      winExt = ".py";
#else
      winExt = ".exe";
#endif
    }

#ifdef CAPTK_PACKAGE_PROJECT
#ifndef _APPLE
    return captk_currentApplicationPath + appName_wrap + winExt;
#else
    return cbica::normPath(captk_currentApplicationPath + "../Resources/bin/" + appName_wrap);
#endif  
#else
    auto individualAppDir = cbica::normPath(captk_currentApplicationPath + "../../src/applications/individualApps/" + appName + "/");
    if (appName_wrap.find("itksnap") != std::string::npos)
    {
      return individualAppDir + "/bin/itksnap" + winExt;
    }
    else
    {
      return individualAppDir + "/" + appName_wrap + winExt;
    }
#endif 
  }

  /**
\brief Get the data directory in the package
*/
  inline std::string getCaPTkDataDir()
  {
    auto captk_dataDir = captk_currentApplicationPath + "../data/";
    if (!cbica::exists(captk_dataDir))
    {
      captk_dataDir = captk_currentApplicationPath + "../../data/";
      if (!cbica::exists(captk_dataDir))
      {
        captk_dataDir = captk_currentApplicationPath + "../Resources/data/";
        if (!cbica::exists(captk_dataDir))
        {
          ShowErrorMessage("Data Directory not found. Please re-install");
          return "";
        }
      }
    }

    return captk_dataDir;
  }