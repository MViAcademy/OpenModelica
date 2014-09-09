/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-2014, Open Source Modelica Consortium (OSMC),
 * c/o Linköpings universitet, Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 LICENSE OR
 * THIS OSMC PUBLIC LICENSE (OSMC-PL) VERSION 1.2.
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES
 * RECIPIENT'S ACCEPTANCE OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3,
 * ACCORDING TO RECIPIENTS CHOICE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from OSMC, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or
 * http://www.openmodelica.org, and in the OpenModelica distribution.
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */
/*
 *
 * @author Adeel Asghar <adeel.asghar@liu.se>
 *
 * RCS: $Id$
 *
 */

#include "LibraryTreeWidget.h"
#include "VariablesWidget.h"

ItemDelegate::ItemDelegate(QObject *pParent, bool drawRichText, bool drawGrid)
  : QItemDelegate(pParent)
{
  mDrawRichText = drawRichText;
  mDrawGrid = drawGrid;
  mpParent = pParent;
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QStyleOptionViewItemV2 opt = setOptions(index, option);
  const QStyleOptionViewItemV2 *v2 = qstyleoption_cast<const QStyleOptionViewItemV2 *>(&option);
  opt.features = v2 ? v2->features : QStyleOptionViewItemV2::ViewItemFeatures(QStyleOptionViewItemV2::None);
  // prepare
  painter->save();
  // get the data and the rectangles
  QVariant value;
  QIcon icon;
  QIcon::Mode iconMode = QIcon::Normal;
  QIcon::State iconState = QIcon::On;
  QPixmap pixmap;
  QRect decorationRect;
  value = index.data(Qt::DecorationRole);
  if (value.isValid())
  {
    if (value.type() == QVariant::Icon)
    {
      icon = qvariant_cast<QIcon>(value);
      decorationRect = QRect(QPoint(0, 0), icon.actualSize(option.decorationSize, iconMode, iconState));
    }
    else
    {
      pixmap = decoration(opt, value);
      decorationRect = QRect(QPoint(0, 0), option.decorationSize).intersected(pixmap.rect());
    }
  }
  QString text;
  QRect displayRect;
  value = index.data(Qt::DisplayRole);
  if (value.isValid())
  {
    if (value.type() == QVariant::Double)
      text = QLocale().toString(value.toDouble());
    else
    {
      text = value.toString();
      text.replace("\n", "<br />");
    }
    displayRect = textRectangle(painter, option.rect, opt.font, text);
  }
  QRect checkRect;
  Qt::CheckState checkState = Qt::Unchecked;
  value = index.data(Qt::CheckStateRole);
  if (value.isValid())
  {
    checkState = static_cast<Qt::CheckState>(value.toInt());
    checkRect = check(opt, opt.rect, value);
  }
  // do the layout
  doLayout(opt, &checkRect, &decorationRect, &displayRect, false);
  // draw the item
  drawBackground(painter, opt, index);
  // hover
  /*
    Ticket #2245. Do not draw hover effect for items. Doesn't seem to work on few versions of Linux.
  */
  /*drawHover(painter, opt, index);*/
  drawCheck(painter, opt, checkRect, checkState);
  /* if draw grid flag is set */
  if (mDrawGrid)
  {
    QPen pen;
    if (!mGridColor.isValid())
    {
      int gridHint = qApp->style()->styleHint(QStyle::SH_Table_GridLineColor, &option);
      const QColor gridColor = static_cast<QRgb>(gridHint);
      pen.setColor(gridColor);
    }
    else
    {
      pen.setColor(mGridColor);
    }
    painter->save();
    painter->setPen(pen);
    painter->drawLine(option.rect.topRight(), option.rect.bottomRight());
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();
  }
  /* if rich text flag is set */
  if (mDrawRichText)
  {
    QTextDocument doc;
    doc.setDefaultFont(opt.font);
    doc.setHtml(text);
    painter->save();
    painter->translate(displayRect.left(), displayRect.top());
    QRect clip(0, 0, displayRect.width(), displayRect.height());
    doc.drawContents(painter, clip);
    painter->restore();
  }
  else
  {
    drawDisplay(painter, opt, displayRect, text);
  }
  if (!icon.isNull())
    icon.paint(painter, decorationRect, option.decorationAlignment, QIcon::Normal, QIcon::Off);
  else
    drawDecoration(painter, opt, decorationRect, pixmap);
  if (parent() && qobject_cast<VariablesTreeView*>(parent()))
  {
    if ((index.column() == 1) && (index.flags() & Qt::ItemIsEditable))
    {
      /* The display rect is slightly larger than the area because it include the outer line. */
      painter->drawRect(displayRect.adjusted(0, 0, -1, -1));
    }
  }
  painter->restore();
}

void ItemDelegate::drawHover(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  if (option.state & QStyle::State_MouseOver)
  {
    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
      cg = QPalette::Inactive;
    painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
  }
}

//! Reimplementation of sizeHint function. Defines the minimum height.
QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  int count = 1;
  int height = 22;
  /* Only calculate the height of the item based on the text for MessagesTreeWidget items. Fix for multi line messages. Ticket #2269. */
  if (parent() && qobject_cast<MessagesTreeWidget*>(parent()))
  {
    QVariant value = index.data(Qt::DisplayRole);
    QString text;
    if (value.isValid())
    {
      if (value.type() == QVariant::Double)
        text = QLocale().toString(value.toDouble());
      else
      {
        text = value.toString();
      }
    }
    count = text.count("\n") + 1;
    height = option.fontMetrics.height() * count + 7; /* 7 is added to add the padding space to the item. */
  }
  QSize size = QItemDelegate::sizeHint(option, index);
  //Set very small height. A minimum apperantly stops at reasonable size.
  size.rheight() = qMax(height, 22); //pixels
  return size;
}

OptionsItemDelegate::OptionsItemDelegate(QObject *pParent)
  : ItemDelegate(pParent)
{

}

//! Reimplementation of sizeHint function. Defines the minimum height.
QSize OptionsItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QSize size = ItemDelegate::sizeHint(option, index);
  //Set very small height. A minimum apperantly stops at reasonable size.
  size.rheight() = 25; //pixels
  return size;
}

SearchClassWidget::SearchClassWidget(MainWindow *pMainWindow)
  : QWidget(pMainWindow)
{
  mpMainWindow = pMainWindow;
  mpSearchClassTextBox = new QLineEdit;
  mpSearchClassTextBox->setPlaceholderText(Helper::searchModelicaClass);
  connect(mpSearchClassTextBox, SIGNAL(returnPressed()), SLOT(searchClasses()));
  mpSearchClassButton = new QPushButton(Helper::search);
  connect(mpSearchClassButton, SIGNAL(clicked()), SLOT(searchClasses()));
  mpFindInModelicaTextCheckBox = new QCheckBox(tr("Within Modelica text"));
  mpNoModelicaClassFoundLabel = new Label(tr("Sorry, no Modelica class found."));
  mpNoModelicaClassFoundLabel->setVisible(false);
  mpLibraryTreeWidget = new LibraryTreeWidget(true, pMainWindow);
  // set grid layout
  QGridLayout *pMainLayout = new QGridLayout;
  pMainLayout->setContentsMargins(0, 0, 0, 0);
  pMainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  pMainLayout->addWidget(mpSearchClassTextBox, 0, 0);
  pMainLayout->addWidget(mpSearchClassButton, 0, 1);
  pMainLayout->addWidget(mpFindInModelicaTextCheckBox, 1, 0, 1, 2);
  pMainLayout->addWidget(mpNoModelicaClassFoundLabel, 2, 0, 1, 2);
  pMainLayout->addWidget(mpLibraryTreeWidget, 3, 0, 1, 2);
  setLayout(pMainLayout);
}

QLineEdit* SearchClassWidget::getSearchClassTextBox()
{
  return mpSearchClassTextBox;
}

void SearchClassWidget::searchClasses()
{
  // Remove the searched classes first
  int i = 0;
  while(i < mpLibraryTreeWidget->topLevelItemCount())
  {
    qDeleteAll(mpLibraryTreeWidget->topLevelItem(i)->takeChildren());
    delete mpLibraryTreeWidget->topLevelItem(i);
    i = 0;   //Restart iteration
  }
  if (mpSearchClassTextBox->text().isEmpty() || (mpSearchClassTextBox->text().compare(Helper::searchModelicaClass) == 0))
    return;
  /* search classes in OMC */
  QStringList searchedClasses = mpMainWindow->getOMCProxy()->searchClassNames(mpSearchClassTextBox->text(),
                                                                              mpFindInModelicaTextCheckBox->isChecked() ? "true" : "false");
  if (searchedClasses.isEmpty())
  {
    mpNoModelicaClassFoundLabel->setVisible(true);
    return;
  }
  else
  {
    mpNoModelicaClassFoundLabel->setVisible(false);
  }
  /* Load the searched classes */
  int progressValue = 0;
  mpMainWindow->getProgressBar()->setRange(0, searchedClasses.size());
  mpMainWindow->showProgressBar();
  for (int j = 0 ; j < searchedClasses.size() ; j++)
  {
    mpMainWindow->getStatusBar()->showMessage(QString(Helper::loading).append(": ").append(searchedClasses[j]));
    LibraryTreeNode *pNewLibraryTreeNode;
    QStringList info = mpMainWindow->getOMCProxy()->getClassInformation(searchedClasses[j]);
    QString fileName = info.size() < 3 ? mpMainWindow->getOMCProxy()->getSourceFile(searchedClasses[j]) : info.at(2);
    StringHandler::ModelicaClasses type = mpMainWindow->getOMCProxy()->getClassRestriction(searchedClasses[j]);
    pNewLibraryTreeNode = new LibraryTreeNode(LibraryTreeNode::Modelica, searchedClasses[j], "", searchedClasses[j],
                                              StringHandler::createTooltip(info, StringHandler::getLastWordAfterDot(searchedClasses[j]), searchedClasses[j]),
                                              type, fileName, !mpLibraryTreeWidget->isFileWritAble(fileName), true, false, mpLibraryTreeWidget);
    bool isDocumentationClass = mpMainWindow->getOMCProxy()->getDocumentationClassAnnotation(searchedClasses[j]);
    pNewLibraryTreeNode->setIsDocumentationClass(isDocumentationClass);
    mpLibraryTreeWidget->loadLibraryComponent(pNewLibraryTreeNode);
    mpLibraryTreeWidget->addTopLevelItem(pNewLibraryTreeNode);
    mpMainWindow->getProgressBar()->setValue(++progressValue);
    mpMainWindow->getStatusBar()->clearMessage();
  }
  mpMainWindow->hideProgressBar();
}

LibraryTreeNode::LibraryTreeNode(LibraryType type, QString text, QString parentName, QString nameStructure, QString tooltip,
                                 StringHandler::ModelicaClasses modelicaType, QString fileName, bool readOnly, bool isSaved, bool isProtected,
                                 LibraryTreeWidget *pParent)
  : mLibraryType(type), mSystemLibrary(false), mpModelWidget(0)
{
  mpLibraryTreeWidget = pParent;
  setModelicaType(modelicaType);
  setName(text);
  setParentName(parentName);
  setNameStructure(nameStructure);
  setText(0, mName);
  /* Do not remove the line below. It is required by LibraryBrowseDialog::useModelicaClass */
  setData(0, Qt::UserRole, nameStructure);
  setToolTip(0, tooltip);
  setIcon(0, getModelicaNodeIcon());
  setFileName(fileName);
  setReadOnly(readOnly);
  setIsSaved(isSaved);
  setIsProtected(isProtected);
  setSaveContentsType(LibraryTreeNode::SaveUnspecified);
}

QIcon LibraryTreeNode::getModelicaNodeIcon()
{
  if (mLibraryType == LibraryTreeNode::Text)
  {
    return QIcon(":/Resources/icons/txt.svg");
  }
  else if (mLibraryType == LibraryTreeNode::TLM)
  {
    return QIcon(":/Resources/icons/class-icon.svg");
  }
  else
  {
    switch (mModelicaType)
    {
      case StringHandler::Model:
        return QIcon(":/Resources/icons/model-icon.svg");
      case StringHandler::Class:
        return QIcon(":/Resources/icons/class-icon.svg");
      case StringHandler::Connector:
        return QIcon(":/Resources/icons/connector-icon.svg");
      case StringHandler::Record:
        return QIcon(":/Resources/icons/record-icon.svg");
      case StringHandler::Block:
        return QIcon(":/Resources/icons/block-icon.svg");
      case StringHandler::Function:
        return QIcon(":/Resources/icons/function-icon.svg");
      case StringHandler::Package:
        return QIcon(":/Resources/icons/package-icon.svg");
      case StringHandler::Type:
      case StringHandler::Operator:
      case StringHandler::OperatorRecord:
      case StringHandler::OperatorFunction:
        return QIcon(":/Resources/icons/type-icon.svg");
      case StringHandler::Optimization:
        return QIcon(":/Resources/icons/optimization-icon.svg");
      default:
        return QIcon(":/Resources/icons/type-icon.svg");
    }
  }
}

void LibraryTreeNode::setModelicaType(StringHandler::ModelicaClasses type)
{
  mModelicaType = type;
}

StringHandler::ModelicaClasses LibraryTreeNode::getModelicaType()
{
  return mModelicaType;
}

void LibraryTreeNode::setName(QString name)
{
  mName = name;
}

const QString& LibraryTreeNode::getName() const
{
  return mName;
}

void LibraryTreeNode::setParentName(QString parentName)
{
  mParentName = parentName;
}

const QString& LibraryTreeNode::getParentName()
{
  return mParentName;
}

void LibraryTreeNode::setNameStructure(QString nameStructure)
{
  mNameStructure = nameStructure;
}

const QString& LibraryTreeNode::getNameStructure()
{
  return mNameStructure;
}

void LibraryTreeNode::setFileName(QString fileName)
{
  // set the filename
  if (!fileName.isEmpty() && !(fileName.compare("<interactive>") == 0))
  {
    fileName = fileName.replace('\\', '/');
    mFileName = fileName;
  }
  else
  {
    mFileName = "";
  }
}

const QString& LibraryTreeNode::getFileName()
{
  return mFileName;
}

void LibraryTreeNode::setReadOnly(bool readOnly)
{
  mReadOnly = readOnly;
}

bool LibraryTreeNode::isReadOnly()
{
  return mReadOnly;
}

void LibraryTreeNode::setSystemLibrary(bool systemLibrary)
{
  mSystemLibrary = systemLibrary;
}

bool LibraryTreeNode::isSystemLibrary()
{
  return mSystemLibrary;
}

void LibraryTreeNode::setIsSaved(bool isSaved)
{
  mIsSaved = isSaved;
}

bool LibraryTreeNode::isSaved()
{
  return mIsSaved;
}

void LibraryTreeNode::setIsProtected(bool isProtected)
{
  mIsProtected = isProtected;
}

bool LibraryTreeNode::isProtected()
{
  return mIsProtected;
}

void LibraryTreeNode::setSaveContentsType(LibraryTreeNode::SaveContentsType saveContentsType)
{
  mSaveContentsType = saveContentsType;
}

LibraryTreeNode::SaveContentsType LibraryTreeNode::getSaveContentsType()
{
  return mSaveContentsType;
}

void LibraryTreeNode::setIsDocumentationClass(bool documentationClass)
{
  mDocumentationClass = documentationClass;
}

bool LibraryTreeNode::isDocumentationClass()
{
  return mDocumentationClass;
}

void LibraryTreeNode::setModelWidget(ModelWidget *pModelWidget)
{
  mpModelWidget = pModelWidget;
}

ModelWidget* LibraryTreeNode::getModelWidget()
{
  return mpModelWidget;
}

LibraryTreeWidget::LibraryTreeWidget(bool isSearchTree, MainWindow *pParent)
  : QTreeWidget(pParent)
{
  mpMainWindow = pParent;
  setObjectName("TreeWithBranches");
  setMinimumWidth(175);
  setItemDelegate(new ItemDelegate(this));
  setTextElideMode(Qt::ElideMiddle);
  setIsSearchedTree(isSearchTree);
  setHeaderLabel(isSearchTree ? tr("Searched Items") : Helper::libraries);
  setIndentation(Helper::treeIndentation);
  setDragEnabled(true);
  setIconSize(Helper::iconSize);
  setColumnCount(1);
  setExpandsOnDoubleClick(false);
  setContextMenuPolicy(Qt::CustomContextMenu);
  createActions();
  connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)), SLOT(expandLibraryTreeNode(QTreeWidgetItem*)));
  connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showContextMenu(QPoint)));
}

LibraryTreeWidget::~LibraryTreeWidget()
{
  // delete all the loaded components
  foreach (LibraryComponent *libraryComponent, mLibraryComponentsList)
  {
    delete libraryComponent;
  }
  // delete all the items in the tree
  for (int i = 0; i < topLevelItemCount(); ++i)
  {
    qDeleteAll(topLevelItem(i)->takeChildren());
    delete topLevelItem(i);
  }
}

MainWindow* LibraryTreeWidget::getMainWindow()
{
  return mpMainWindow;
}

void LibraryTreeWidget::setIsSearchedTree(bool isSearchTree)
{
  mIsSearchTree = isSearchTree;
}

bool LibraryTreeWidget::isSearchedTree()
{
  return mIsSearchTree;
}

void LibraryTreeWidget::addToExpandedLibraryTreeNodesList(LibraryTreeNode *pLibraryTreeNode)
{
  mExpandedLibraryTreeNodesList.append(pLibraryTreeNode);
}

void LibraryTreeWidget::removeFromExpandedLibraryTreeNodesList(LibraryTreeNode *pLibraryTreeNode)
{
  mExpandedLibraryTreeNodesList.removeOne(pLibraryTreeNode);
}

void LibraryTreeWidget::createActions()
{
  // show Model Action
  mpViewClassAction = new QAction(QIcon(":/Resources/icons/modeling.png"), Helper::viewClass, this);
  mpViewClassAction->setStatusTip(Helper::viewClassTip);
  connect(mpViewClassAction, SIGNAL(triggered()), SLOT(showModelWidget()));
  // view documentation Action
  mpViewDocumentationAction = new QAction(QIcon(":/Resources/icons/info-icon.png"), Helper::viewDocumentation, this);
  mpViewDocumentationAction->setStatusTip(Helper::viewDocumentationTip);
  connect(mpViewDocumentationAction, SIGNAL(triggered()), SLOT(viewDocumentation()));
  // new Modelica Class Action
  mpNewModelicaClassAction = new QAction(QIcon(":/Resources/icons/new.svg"), Helper::newModelicaClass, this);
  mpNewModelicaClassAction->setStatusTip(Helper::createNewModelicaClass);
  connect(mpNewModelicaClassAction, SIGNAL(triggered()), SLOT(createNewModelicaClass()));
  // instantiate Model Action
  mpInstantiateModelAction = new QAction(QIcon(":/Resources/icons/flatmodel.png"), Helper::instantiateModel, this);
  mpInstantiateModelAction->setStatusTip(Helper::instantiateModelTip);
  connect(mpInstantiateModelAction, SIGNAL(triggered()), SLOT(instantiateModel()));
  // check Model Action
  mpCheckModelAction = new QAction(QIcon(":/Resources/icons/check.svg"), Helper::checkModel, this);
  mpCheckModelAction->setStatusTip(Helper::checkModelTip);
  connect(mpCheckModelAction, SIGNAL(triggered()), SLOT(checkModel()));
  // check all Models Action
  mpCheckAllModelsAction = new QAction(QIcon(":/Resources/icons/check-all.svg"), Helper::checkAllModels, this);
  mpCheckAllModelsAction->setStatusTip(Helper::checkAllModelsTip);
  connect(mpCheckAllModelsAction, SIGNAL(triggered()), SLOT(checkAllModels()));
  // simulate Action
  mpSimulateAction = new QAction(QIcon(":/Resources/icons/simulate.png"), Helper::simulate, this);
  mpSimulateAction->setStatusTip(Helper::simulateTip);
  connect(mpSimulateAction, SIGNAL(triggered()), SLOT(simulate()));
  // simulation setup Action
  mpSimulationSetupAction = new QAction(QIcon(":/Resources/icons/simulation-center.svg"), Helper::simulationSetup, this);
  mpSimulationSetupAction->setStatusTip(Helper::simulationSetupTip);
  connect(mpSimulationSetupAction, SIGNAL(triggered()), SLOT(simulationSetup()));
  // unload Action
  mpUnloadClassAction = new QAction(QIcon(":/Resources/icons/delete.svg"), Helper::unloadClass, this);
  mpUnloadClassAction->setStatusTip(Helper::unloadClassTip);
  connect(mpUnloadClassAction, SIGNAL(triggered()), SLOT(unloadClass()));
  // unload text file Action
  mpUnloadTextFileAction = new QAction(QIcon(":/Resources/icons/delete.svg"), Helper::unloadClass, this);
  mpUnloadTextFileAction->setStatusTip(Helper::unloadClassTip);
  connect(mpUnloadTextFileAction, SIGNAL(triggered()), SLOT(unloadTextFile()));
  // unload xml file Action
  mpUnloadXMLFileAction = new QAction(QIcon(":/Resources/icons/delete.svg"), Helper::unloadClass, this);
  mpUnloadXMLFileAction->setStatusTip(Helper::unloadXMLTip);
  connect(mpUnloadXMLFileAction, SIGNAL(triggered()), SLOT(unloadXMLFile()));
  // refresh Action
  mpRefreshAction = new QAction(QIcon(":/Resources/icons/refresh.png"), Helper::refresh, this);
  mpRefreshAction->setStatusTip(tr("Refresh the Modelica class"));
  connect(mpRefreshAction, SIGNAL(triggered()), SLOT(refresh()));
  // Export FMU Action
  mpExportFMUAction = new QAction(QIcon(":/Resources/icons/export-fmu.svg"), Helper::exportFMU, this);
  mpExportFMUAction->setStatusTip(Helper::exportFMUTip);
  connect(mpExportFMUAction, SIGNAL(triggered()), SLOT(exportModelFMU()));
  // Export XML Action
  mpExportXMLAction = new QAction(QIcon(":/Resources/icons/export-xml.svg"), Helper::exportXML, this);
  mpExportXMLAction->setStatusTip(Helper::exportXMLTip);
  connect(mpExportXMLAction, SIGNAL(triggered()), SLOT(exportModelXML()));
  // Export Figaro Action
  mpExportFigaroAction = new QAction(QIcon(":/Resources/icons/console.png"), Helper::exportFigaro, this);
  mpExportFigaroAction->setStatusTip(Helper::exportFigaroTip);
  connect(mpExportFigaroAction, SIGNAL(triggered()), SLOT(exportModelFigaro()));
}

//! Let the user add the OM Standard Library to library widget.
void LibraryTreeWidget::addModelicaLibraries(QSplashScreen *pSplashScreen)
{
  // load Modelica System Libraries.
  mpMainWindow->getOMCProxy()->loadSystemLibraries(pSplashScreen);
  pSplashScreen->showMessage(tr("Creating Components"), Qt::AlignRight, Qt::white);
  QStringList systemLibs = mpMainWindow->getOMCProxy()->getClassNames("");
  systemLibs.prepend("OpenModelica");
  systemLibs.sort();
  foreach (QString lib, systemLibs)
  {
    QStringList info = mpMainWindow->getOMCProxy()->getClassInformation(lib);
    StringHandler::ModelicaClasses type = info.size() < 3 ? mpMainWindow->getOMCProxy()->getClassRestriction(lib) : StringHandler::getModelicaClassType(info.at(0));
    QString fileName = info.size() < 3 ? mpMainWindow->getOMCProxy()->getSourceFile(lib) : info.at(2);
    LibraryTreeNode *pNewLibraryTreeNode = new LibraryTreeNode(LibraryTreeNode::Modelica, lib, QString(""), lib, StringHandler::createTooltip(info, lib, lib), type,
                                                               fileName, true, true, false, this);
    pNewLibraryTreeNode->setSystemLibrary(true);
    bool isDocumentationClass = mpMainWindow->getOMCProxy()->getDocumentationClassAnnotation(lib);
    pNewLibraryTreeNode->setIsDocumentationClass(isDocumentationClass);
    // get the Icon for Modelica tree node
    loadLibraryComponent(pNewLibraryTreeNode);
    addTopLevelItem(pNewLibraryTreeNode);
    mLibraryTreeNodesList.append(pNewLibraryTreeNode);
    createLibraryTreeNodes(pNewLibraryTreeNode);
  }
  // load Modelica User Libraries.
  mpMainWindow->getOMCProxy()->loadUserLibraries(pSplashScreen);
  QStringList userLibs = mpMainWindow->getOMCProxy()->getClassNames("");
  foreach (QString lib, userLibs)
  {
    if (systemLibs.contains(lib))
      continue;
    QStringList info = mpMainWindow->getOMCProxy()->getClassInformation(lib);
    StringHandler::ModelicaClasses type = info.size() < 3 ? mpMainWindow->getOMCProxy()->getClassRestriction(lib) : StringHandler::getModelicaClassType(info.at(0));
    QString fileName = info.size() < 3 ? mpMainWindow->getOMCProxy()->getSourceFile(lib) : info.at(2);
    LibraryTreeNode *pNewLibraryTreeNode = new LibraryTreeNode(LibraryTreeNode::Modelica, lib, QString(""), lib, StringHandler::createTooltip(info, lib, lib), type,
                                                               fileName, !isFileWritAble(fileName), true, false, this);
    bool isDocumentationClass = mpMainWindow->getOMCProxy()->getDocumentationClassAnnotation(lib);
    pNewLibraryTreeNode->setIsDocumentationClass(isDocumentationClass);
    // get the Icon for Modelica tree node
    loadLibraryComponent(pNewLibraryTreeNode);
    addTopLevelItem(pNewLibraryTreeNode);
    mLibraryTreeNodesList.append(pNewLibraryTreeNode);
    createLibraryTreeNodes(pNewLibraryTreeNode);
  }
}

void LibraryTreeWidget::createLibraryTreeNodes(LibraryTreeNode *pLibraryTreeNode)
{
  QStringList libs = mpMainWindow->getOMCProxy()->getClassNames(pLibraryTreeNode->getNameStructure(), "true");
  if (!libs.isEmpty())
    libs.removeFirst();
  QList<LibraryTreeNode*> nodes;
  foreach (QString lib, libs)
  {
    /* $Code is a special OpenModelica keyword. No API command will work if we use it. */
    if (lib.contains("$Code"))
      continue;
    QString name = StringHandler::getLastWordAfterDot(lib);
    QString parentName = StringHandler::removeLastWordAfterDot(lib);
    QStringList info = mpMainWindow->getOMCProxy()->getClassInformation(lib);
    StringHandler::ModelicaClasses type = info.size() < 3 ? mpMainWindow->getOMCProxy()->getClassRestriction(lib) : StringHandler::getModelicaClassType(info.at(0));
    QString fileName = info.size() < 3 ? mpMainWindow->getOMCProxy()->getSourceFile(lib) : info.at(2);
    LibraryTreeNode *pNewLibraryTreeNode = new LibraryTreeNode(LibraryTreeNode::Modelica, name, parentName, lib, StringHandler::createTooltip(info, lib, lib), type,
                                                               fileName, !isFileWritAble(fileName), pLibraryTreeNode->isSaved(), false, this);
    pNewLibraryTreeNode->setSystemLibrary(pLibraryTreeNode->isSystemLibrary());
    LibraryTreeNode *pParentLibraryTreeNode = getLibraryTreeNode(parentName);
    if (pParentLibraryTreeNode->isDocumentationClass())
    {
      pNewLibraryTreeNode->setIsDocumentationClass(true);
    }
    else
    {
      bool isDocumentationClass = mpMainWindow->getOMCProxy()->getDocumentationClassAnnotation(lib);
      pNewLibraryTreeNode->setIsDocumentationClass(isDocumentationClass);
    }
    nodes.append(pNewLibraryTreeNode);
    mLibraryTreeNodesList.append(pNewLibraryTreeNode);
  }
  addLibraryTreeNodes(nodes);
}

void LibraryTreeWidget::expandLibraryTreeNode(LibraryTreeNode *pLibraryTreeNode)
{
  // set the range for progress bar.
  int progressValue = 0;
  mpMainWindow->getProgressBar()->setRange(0, pLibraryTreeNode->childCount());
  mpMainWindow->showProgressBar();
  for (int i = 0 ; i < pLibraryTreeNode->childCount() ; i++)
  {
    loadLibraryTreeNode(pLibraryTreeNode, dynamic_cast<LibraryTreeNode*>(pLibraryTreeNode->child(i)));
    mpMainWindow->getProgressBar()->setValue(++progressValue);
  }
  mpMainWindow->hideProgressBar();
}

void LibraryTreeWidget::loadLibraryTreeNode(LibraryTreeNode *pParentLibraryTreeNode, LibraryTreeNode *pLibraryTreeNode)
{
  QString className = pLibraryTreeNode->getNameStructure();
  QString parentName = pParentLibraryTreeNode->getNameStructure();
  QString name = pLibraryTreeNode->getName();
  mpMainWindow->getStatusBar()->showMessage(QString(Helper::loading).append(": ").append(className));
  QStringList info = mpMainWindow->getOMCProxy()->getClassInformation(className);
  StringHandler::ModelicaClasses type = info.size() < 3 ? mpMainWindow->getOMCProxy()->getClassRestriction(className) : StringHandler::getModelicaClassType(info.at(0));
  QString fileName = info.size() < 3 ? mpMainWindow->getOMCProxy()->getSourceFile(className) : info.at(2);
  bool isProtected = mpMainWindow->getOMCProxy()->isProtectedClass(parentName, name);
  // update LibraryTreeNode attributes
  pLibraryTreeNode->setModelicaType(type);
  pLibraryTreeNode->setToolTip(0, StringHandler::createTooltip(info, name, className));
  pLibraryTreeNode->setIcon(0, pLibraryTreeNode->getModelicaNodeIcon());
  pLibraryTreeNode->setFileName(fileName);
  pLibraryTreeNode->setReadOnly(!isFileWritAble(fileName));
  pLibraryTreeNode->setIsSaved(pParentLibraryTreeNode->isSaved());
  pLibraryTreeNode->setIsProtected(isProtected);
  if (isProtected)
    pLibraryTreeNode->setHidden(!getMainWindow()->getOptionsDialog()->getGeneralSettingsPage()->getShowProtectedClasses());
  // load the library icon
  loadLibraryComponent(pLibraryTreeNode);
}

void LibraryTreeWidget::addLibraryTreeNodes(QList<LibraryTreeNode *> libraryTreeNodes)
{
  foreach (LibraryTreeNode *pLibraryTreeNode, libraryTreeNodes)
  {
    if (pLibraryTreeNode->getParentName().isEmpty())
    {
      addTopLevelItem(pLibraryTreeNode);
    }
    else
    {
      QString parentName = StringHandler::removeLastWordAfterDot(pLibraryTreeNode->getNameStructure());
      for (int i = 0 ; i < mLibraryTreeNodesList.size() ; i++)
      {
        if (mLibraryTreeNodesList[i]->getNameStructure().compare(parentName) == 0)
        {
          mLibraryTreeNodesList[i]->addChild(pLibraryTreeNode);
          break;
        }
      }
    }
  }
}

bool LibraryTreeWidget::isLibraryTreeNodeExpanded(QTreeWidgetItem *item)
{
  foreach (LibraryTreeNode *pLibraryTreeNode, mExpandedLibraryTreeNodesList)
  {
    LibraryTreeNode *pItem = dynamic_cast<LibraryTreeNode*>(item);
    if (pLibraryTreeNode == pItem)
      return true;
  }
  return false;
}

bool LibraryTreeWidget::sortNodesAscending(const LibraryTreeNode *node1, const LibraryTreeNode *node2)
{
  return node1->getName().toLower() < node2->getName().toLower();
}

LibraryTreeNode* LibraryTreeWidget::addLibraryTreeNode(QString name, StringHandler::ModelicaClasses type, QString parentName,
                                                       bool isSaved, int insertIndex)
{
  LibraryTreeNode *pNewLibraryTreeNode;
  QString className = parentName.isEmpty() ? name : QString(parentName).append(".").append(name);
  mpMainWindow->getStatusBar()->showMessage(QString(Helper::loading).append(": ").append(className));
  QStringList info = mpMainWindow->getOMCProxy()->getClassInformation(className);
  QString fileName = info.size() < 3 ? mpMainWindow->getOMCProxy()->getSourceFile(className) : info.at(2);
  pNewLibraryTreeNode = new LibraryTreeNode(LibraryTreeNode::Modelica, name, parentName, className,
                                            StringHandler::createTooltip(info, name, className), type, fileName, !isFileWritAble(fileName),
                                            isSaved, false, this);
  if (parentName.isEmpty())
  {
    if (insertIndex == 0)
      addTopLevelItem(pNewLibraryTreeNode);
    else
      insertTopLevelItem(insertIndex, pNewLibraryTreeNode);
    bool isDocumentationClass = mpMainWindow->getOMCProxy()->getDocumentationClassAnnotation(className);
    pNewLibraryTreeNode->setIsDocumentationClass(isDocumentationClass);
  }
  else
  {
    LibraryTreeNode *pLibraryTreeNode = getLibraryTreeNode(parentName);
    if (insertIndex == 0)
      pLibraryTreeNode->addChild(pNewLibraryTreeNode);
    else
      pLibraryTreeNode->insertChild(insertIndex, pNewLibraryTreeNode);
    if (pLibraryTreeNode->isDocumentationClass())
    {
      pNewLibraryTreeNode->setIsDocumentationClass(true);
    }
    else
    {
      bool isDocumentationClass = mpMainWindow->getOMCProxy()->getDocumentationClassAnnotation(className);
      pNewLibraryTreeNode->setIsDocumentationClass(isDocumentationClass);
    }
  }
  // load the models icon
  loadLibraryComponent(pNewLibraryTreeNode);
  mLibraryTreeNodesList.append(pNewLibraryTreeNode);
  mpMainWindow->getStatusBar()->clearMessage();
  return pNewLibraryTreeNode;
}

LibraryTreeNode* LibraryTreeWidget::addLibraryTreeNode(QString name, bool isSaved, int insertIndex)
{
  mpMainWindow->getStatusBar()->showMessage(QString(Helper::loading).append(": ").append(name));
  QString fileName = "";
  LibraryTreeNode *pNewLibraryTreeNode = new LibraryTreeNode(LibraryTreeNode::TLM, name, "", name, StringHandler::createTooltip(name, ""),
                                                             StringHandler::Model, fileName, !isFileWritAble(fileName), isSaved, false, this);
  pNewLibraryTreeNode->setIsDocumentationClass(false);

  if (insertIndex == 0)
    addTopLevelItem(pNewLibraryTreeNode);
  else
    insertTopLevelItem(insertIndex, pNewLibraryTreeNode);

  mLibraryTreeNodesList.append(pNewLibraryTreeNode);
  mpMainWindow->getStatusBar()->clearMessage();
  return pNewLibraryTreeNode;
}

LibraryTreeNode* LibraryTreeWidget::getLibraryTreeNode(QString nameStructure, Qt::CaseSensitivity caseSensitivity)
{
  for (int i = 0 ; i < mLibraryTreeNodesList.size() ; i++)
  {
    if (mLibraryTreeNodesList[i]->getNameStructure().compare(nameStructure, caseSensitivity) == 0)
    {
      return mLibraryTreeNodesList[i];
    }
  }
  return 0;
}

QList<LibraryTreeNode*> LibraryTreeWidget::getLibraryTreeNodesList()
{
  return mLibraryTreeNodesList;
}

void LibraryTreeWidget::addLibraryComponentObject(LibraryComponent *libraryComponent)
{
  mLibraryComponentsList.append(libraryComponent);
}

Component* LibraryTreeWidget::getComponentObject(QString className)
{
  foreach (LibraryComponent *pLibraryComponent, mLibraryComponentsList)
  {
    if (pLibraryComponent->mClassName == className)
      return pLibraryComponent->mpComponent;
  }
  return 0;
}

LibraryComponent* LibraryTreeWidget::getLibraryComponentObject(QString className)
{
  foreach (LibraryComponent *pLibraryComponent, mLibraryComponentsList)
  {
    if (pLibraryComponent->mClassName == className)
    {
      return pLibraryComponent;
    }
  }
  return 0;
}

bool LibraryTreeWidget::isFileWritAble(QString filePath)
{
  QFile file(filePath);
  if (file.exists())
    return file.permissions().testFlag(QFile::WriteUser);
  else
    return true;
}

void LibraryTreeWidget::showProtectedClasses(bool enable)
{
  for (int i = 0 ; i < mLibraryTreeNodesList.size() ; i++)
  {
    if (mLibraryTreeNodesList[i]->isProtected())
      mLibraryTreeNodesList[i]->setHidden(!enable);
  }
}

bool LibraryTreeWidget::unloadClass(LibraryTreeNode *pLibraryTreeNode, bool askQuestion)
{
  if (askQuestion)
  {
    QMessageBox *pMessageBox = new QMessageBox(mpMainWindow);
    pMessageBox->setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::question));
    pMessageBox->setIcon(QMessageBox::Question);
    pMessageBox->setText(GUIMessages::getMessage(GUIMessages::DELETE_CLASS_MSG).arg(pLibraryTreeNode->getNameStructure()));
    pMessageBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    pMessageBox->setDefaultButton(QMessageBox::Yes);
    int answer = pMessageBox->exec();
    switch (answer)
    {
      case QMessageBox::Yes:
        // Yes was clicked. Don't return.
        break;
      case QMessageBox::No:
        // No was clicked. Return
        return false;
      default:
        // should never be reached
        return false;
    }
  }
  /*
    Delete the class in OMC.
    If deleteClass is successfull remove the class from Library Browser and delete the corresponding ModelWidget.
    */
  if (mpMainWindow->getOMCProxy()->deleteClass(pLibraryTreeNode->getNameStructure()))
  {
    /* remove the child nodes first */
    unloadClassHelper(pLibraryTreeNode);
    mpMainWindow->getOMCProxy()->removeCachedOMCCommand(pLibraryTreeNode->getNameStructure());
    unloadLibraryTreeNodeAndModelWidget(pLibraryTreeNode);
    return true;
  }
  else
  {
    QMessageBox::critical(mpMainWindow, QString(Helper::applicationName).append(" - ").append(Helper::error),
                          GUIMessages::getMessage(GUIMessages::ERROR_OCCURRED).arg(mpMainWindow->getOMCProxy()->getResult())
                          .append(tr("while deleting ") + pLibraryTreeNode->getNameStructure()), Helper::ok);
    return false;
  }
}

bool LibraryTreeWidget::unloadTextFile(LibraryTreeNode *pLibraryTreeNode, bool askQuestion)
{
  if (askQuestion)
  {
    QMessageBox *pMessageBox = new QMessageBox(mpMainWindow);
    pMessageBox->setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::question));
    pMessageBox->setIcon(QMessageBox::Question);
    pMessageBox->setText(GUIMessages::getMessage(GUIMessages::DELETE_TEXT_FILE_MSG).arg(pLibraryTreeNode->getNameStructure()));
    pMessageBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    pMessageBox->setDefaultButton(QMessageBox::Yes);
    int answer = pMessageBox->exec();
    switch (answer)
    {
      case QMessageBox::Yes:
        // Yes was clicked. Don't return.
        break;
      case QMessageBox::No:
        // No was clicked. Return
        return false;
      default:
        // should never be reached
        return false;
    }
  }
  unloadLibraryTreeNodeAndModelWidget(pLibraryTreeNode);
  return true;
}

bool LibraryTreeWidget::unloadXMLFile(LibraryTreeNode *pLibraryTreeNode, bool askQuestion)
{
  if (askQuestion)
  {
    QMessageBox *pMessageBox = new QMessageBox(mpMainWindow);
    pMessageBox->setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::question));
    pMessageBox->setIcon(QMessageBox::Question);
    pMessageBox->setText(GUIMessages::getMessage(GUIMessages::DELETE_TEXT_FILE_MSG).arg(pLibraryTreeNode->getNameStructure()));
    pMessageBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    pMessageBox->setDefaultButton(QMessageBox::Yes);
    int answer = pMessageBox->exec();
    switch (answer)
    {
      case QMessageBox::Yes:
        // Yes was clicked. Don't return.
        break;
      case QMessageBox::No:
        // No was clicked. Return
        return false;
      default:
        // should never be reached
        return false;
    }
  }
  unloadLibraryTreeNodeAndModelWidget(pLibraryTreeNode);
  return true;
}

void LibraryTreeWidget::unloadClassHelper(LibraryTreeNode *pLibraryTreeNode)
{
  for (int i = 0 ; i < pLibraryTreeNode->childCount(); i++)
  {
    LibraryTreeNode *pChildLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(pLibraryTreeNode->child(i));
    /* remove the ModelWidget of LibraryTreeNode and remove the QMdiSubWindow from MdiArea and delete it. */
    if (pChildLibraryTreeNode->getModelWidget())
    {
      QMdiSubWindow *pMdiSubWindow = mpMainWindow->getModelWidgetContainer()->getMdiSubWindow(pChildLibraryTreeNode->getModelWidget());
      if (pMdiSubWindow)
      {
        pMdiSubWindow->close();
        pMdiSubWindow->deleteLater();
      }
      pChildLibraryTreeNode->getModelWidget()->deleteLater();
    }
    mpMainWindow->getOMCProxy()->removeCachedOMCCommand(pChildLibraryTreeNode->getNameStructure());
    mLibraryTreeNodesList.removeOne(pChildLibraryTreeNode);
    mExpandedLibraryTreeNodesList.removeOne(pChildLibraryTreeNode);
    unloadClassHelper(pChildLibraryTreeNode);
  }
}

bool LibraryTreeWidget::saveLibraryTreeNode(LibraryTreeNode *pLibraryTreeNode)
{
  bool result = false;
  mpMainWindow->getStatusBar()->showMessage(tr("Saving %1").arg(pLibraryTreeNode->getNameStructure()));
  mpMainWindow->showProgressBar();
  if (pLibraryTreeNode->getLibraryType() == LibraryTreeNode::Modelica)
  {
    result = saveModelicaLibraryTreeNode(pLibraryTreeNode);
  }
  else if (pLibraryTreeNode->getLibraryType() == LibraryTreeNode::TLM)
  {
    result = saveXMLLibraryTreeNode(pLibraryTreeNode);
  }
  else if (pLibraryTreeNode->getLibraryType() == LibraryTreeNode::Text)
  {
    result = saveTextLibraryTreeNode(pLibraryTreeNode);
  }
  mpMainWindow->getStatusBar()->clearMessage();
  mpMainWindow->hideProgressBar();
  return result;
}

LibraryTreeNode* LibraryTreeWidget::findParentLibraryTreeNodeSavedInSameFile(LibraryTreeNode *pLibraryTreeNode, QFileInfo fileInfo)
{
  LibraryTreeNode *pParentLibraryTreeNode = getLibraryTreeNode(pLibraryTreeNode->getParentName());
  if (pParentLibraryTreeNode)
  {
    QFileInfo libraryTreeNodeFileInfo(pParentLibraryTreeNode->getFileName());
    if (fileInfo.absoluteFilePath().compare(libraryTreeNodeFileInfo.absoluteFilePath()) == 0)
      return findParentLibraryTreeNodeSavedInSameFile(pParentLibraryTreeNode, fileInfo);
    else
      return pLibraryTreeNode;
  }
  else
  {
    return pLibraryTreeNode;
  }
}

bool LibraryTreeWidget::saveModelicaLibraryTreeNode(LibraryTreeNode *pLibraryTreeNode)
{
  bool result = false;
  if (pLibraryTreeNode->getParentName().isEmpty() && pLibraryTreeNode->childCount() == 0)
  {
    /*
      A root model with no sub models.
      If it is a package then check whether save contents type. Otherwise simply save it to file.
      */
    if (pLibraryTreeNode->getModelicaType() == StringHandler::Package && pLibraryTreeNode->getSaveContentsType() == LibraryTreeNode::SaveInOneFile)
    {
      result = saveLibraryTreeNodeOneFileHelper(pLibraryTreeNode);
    }
    else if (pLibraryTreeNode->getModelicaType() == StringHandler::Package && pLibraryTreeNode->getSaveContentsType() == LibraryTreeNode::SaveFolderStructure)
    {
      result = saveLibraryTreeNodeFolderHelper(pLibraryTreeNode);
    }
    else
    {
      result = saveLibraryTreeNodeHelper(pLibraryTreeNode);
    }
    if (result)
      getMainWindow()->addRecentFile(pLibraryTreeNode->getFileName(), Helper::utf8);
  }
  else if (pLibraryTreeNode->getParentName().isEmpty() && pLibraryTreeNode->childCount() > 0)
  {
    /* A root model with sub models.
     * If its a new model then its fileName is <interactive> then check its mSaveContentsType.
     * If mSaveContentsType is LibraryTreeNode::SaveInOneFile then we save all sub models in one file
     */
    if (pLibraryTreeNode->getFileName().isEmpty() && pLibraryTreeNode->getSaveContentsType() == LibraryTreeNode::SaveInOneFile)
    {
      result = saveLibraryTreeNodeOneFileHelper(pLibraryTreeNode);
    }
    /* A root model with sub models.
     * If its a new model then its fileName is <interactive> then check its mSaveContentsType.
     * If mSaveContentsType is LibraryTreeNode::SaveFolderStructure then we save sub models in folder structure
     */
    else if (pLibraryTreeNode->getFileName().isEmpty() && pLibraryTreeNode->getSaveContentsType() == LibraryTreeNode::SaveFolderStructure)
    {
      result = saveLibraryTreeNodeFolderHelper(pLibraryTreeNode);
    }
    else
    {
      result = saveLibraryTreeNodeOneFileOrFolderHelper(pLibraryTreeNode);
    }
    if (result)
      getMainWindow()->addRecentFile(pLibraryTreeNode->getFileName(), Helper::utf8);
  }
  else if (!pLibraryTreeNode->getParentName().isEmpty())
  {
    /* A sub model contained inside some other model.
     * Find its root model.
     * If the root model fileName is <interactive> then check its mSaveContentsType.
     * If mSaveContentsType is LibraryTreeNode::SaveInOneFile then we save all sub models in one file.
     */
    pLibraryTreeNode = getLibraryTreeNode(StringHandler::getFirstWordBeforeDot(pLibraryTreeNode->getNameStructure()));
    if (pLibraryTreeNode->getFileName().isEmpty() && pLibraryTreeNode->getSaveContentsType() == LibraryTreeNode::SaveInOneFile)
    {
      result = saveLibraryTreeNodeOneFileHelper(pLibraryTreeNode);
    }
    /* A sub model contained inside some other model.
     * Find its root model.
     * If its a new model then its fileName is <interactive> then check its mSaveContentsType.
     * If mSaveContentsType is LibraryTreeNode::SaveFolderStructure then we save sub models in folder structure
     */
    else if (pLibraryTreeNode->getFileName().isEmpty() && pLibraryTreeNode->getSaveContentsType() == LibraryTreeNode::SaveFolderStructure)
    {
      result = saveLibraryTreeNodeFolderHelper(pLibraryTreeNode);
    }
    else
    {
      result = saveLibraryTreeNodeOneFileOrFolderHelper(pLibraryTreeNode);
    }
    if (result)
      getMainWindow()->addRecentFile(pLibraryTreeNode->getFileName(), Helper::utf8);
  }
}

bool LibraryTreeWidget::saveTextLibraryTreeNode(LibraryTreeNode *pLibraryTreeNode)
{
  QString fileName;
  if (pLibraryTreeNode->getFileName().isEmpty())
  {
    QString name = pLibraryTreeNode->getName();
    fileName = StringHandler::getSaveFileName(this, QString(Helper::applicationName).append(" - ").append(tr("Save File")), NULL,
                                              Helper::txtFileTypes, NULL, "txt", &name);
    if (fileName.isEmpty())   // if user press ESC
      return false;
  }
  else
  {
    fileName = pLibraryTreeNode->getFileName();
  }

  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream textStream(&file);
    textStream << pLibraryTreeNode->getModelWidget()->getTextEditor()->toPlainText();
    file.close();
    /* mark the file as saved and update the labels. */
    pLibraryTreeNode->setIsSaved(true);
    pLibraryTreeNode->setFileName(fileName);
    if (pLibraryTreeNode->getModelWidget())
    {
      pLibraryTreeNode->getModelWidget()->setWindowTitle(pLibraryTreeNode->getNameStructure());
      pLibraryTreeNode->getModelWidget()->setModelFilePathLabel(fileName);
    }
  } else {
    QMessageBox::information(this, Helper::applicationName + " - " + Helper::error, GUIMessages::getMessage(GUIMessages::ERROR_OCCURRED)
                             .arg(tr("Unable to save the file. %1").arg(file.errorString())), Helper::ok);
    return false;
  }
  return true;
}

bool LibraryTreeWidget::saveXMLLibraryTreeNode(LibraryTreeNode *pLibraryTreeNode)
{
  QString fileName;
  if (pLibraryTreeNode->getFileName().isEmpty())
  {
    QString name = pLibraryTreeNode->getName();
    fileName = StringHandler::getSaveFileName(this, QString(Helper::applicationName).append(" - ").append(tr("Save File")), NULL,
                                              Helper::xmlFileTypes, NULL, "xml", &name);
    if (fileName.isEmpty())   // if user press ESC
      return false;
  }
  else
  {
    fileName = pLibraryTreeNode->getFileName();
  }

  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream textStream(&file);
    textStream << pLibraryTreeNode->getModelWidget()->getTLMEditor()->toPlainText();
    file.close();
    /* mark the file as saved and update the labels. */
    pLibraryTreeNode->setIsSaved(true);
    pLibraryTreeNode->setFileName(fileName);
    if (pLibraryTreeNode->getModelWidget())
    {
      pLibraryTreeNode->getModelWidget()->setWindowTitle(pLibraryTreeNode->getNameStructure());
      pLibraryTreeNode->getModelWidget()->setModelFilePathLabel(fileName);
    }
  } else {
    QMessageBox::information(this, Helper::applicationName + " - " + Helper::error, GUIMessages::getMessage(GUIMessages::ERROR_OCCURRED)
                             .arg(tr("Unable to save the file. %1").arg(file.errorString())), Helper::ok);
    return false;
  }
  return true;
}

bool LibraryTreeWidget::saveLibraryTreeNodeHelper(LibraryTreeNode *pLibraryTreeNode)
{
  mpMainWindow->getStatusBar()->showMessage(QString(tr("Saving")).append(" ").append(pLibraryTreeNode->getNameStructure()));
  QString fileName;
  if (pLibraryTreeNode->getFileName().isEmpty())
  {
    QString name = pLibraryTreeNode->getName();
    fileName = StringHandler::getSaveFileName(this, QString(Helper::applicationName).append(" - ").append(tr("Save File")), NULL,
                                              Helper::omFileTypes, NULL, "mo", &name);
    if (fileName.isEmpty())   // if user press ESC
      return false;
  }
  else
  {
    fileName = pLibraryTreeNode->getFileName();
  }
  /* if user has done some changes in the Modelica text view then save & validate it in the AST before saving it to file. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return false;
  }
  mpMainWindow->getOMCProxy()->setSourceFile(pLibraryTreeNode->getNameStructure(), fileName);
  // save the model through OMC
  if (mpMainWindow->getOMCProxy()->save(pLibraryTreeNode->getNameStructure(), fileName))
  {
    pLibraryTreeNode->setIsSaved(true);
    pLibraryTreeNode->setFileName(fileName);
    if (pLibraryTreeNode->getModelWidget())
    {
      pLibraryTreeNode->getModelWidget()->setWindowTitle(pLibraryTreeNode->getNameStructure());
      pLibraryTreeNode->getModelWidget()->setModelFilePathLabel(fileName);
    }
    return true;
  }
  return false;
}

bool LibraryTreeWidget::saveLibraryTreeNodeOneFileHelper(LibraryTreeNode *pLibraryTreeNode)
{
  mpMainWindow->getStatusBar()->showMessage(QString(tr("Saving")).append(" ").append(pLibraryTreeNode->getNameStructure()));
  QString fileName;
  if (pLibraryTreeNode->getFileName().isEmpty())
  {
    QString name = pLibraryTreeNode->getName();
    fileName = StringHandler::getSaveFileName(this, QString(Helper::applicationName).append(" - ").append(tr("Save File")), NULL,
                                              Helper::omFileTypes, NULL, "mo", &name);
    if (fileName.isEmpty())   // if user press ESC
      return false;
  }
  else
  {
    fileName = pLibraryTreeNode->getFileName();
  }
  // set the fileName for the model.
  mpMainWindow->getOMCProxy()->setSourceFile(pLibraryTreeNode->getNameStructure(), fileName);
  // set the fileName for the sub models
  if (!setSubModelsFileNameOneFileHelper(pLibraryTreeNode, fileName))
    return false;
  // save the model through OMC
  if (mpMainWindow->getOMCProxy()->save(pLibraryTreeNode->getNameStructure(), fileName))
  {
    pLibraryTreeNode->setIsSaved(true);
    pLibraryTreeNode->setFileName(fileName);
    if (pLibraryTreeNode->getModelWidget())
    {
      pLibraryTreeNode->getModelWidget()->setWindowTitle(pLibraryTreeNode->getNameStructure());
      pLibraryTreeNode->getModelWidget()->setModelFilePathLabel(fileName);
    }
    setSubModelsSavedOneFileHelper(pLibraryTreeNode);
    return true;
  }
  else
  {
    mpMainWindow->getOMCProxy()->printMessagesStringInternal();
    return false;
  }
}

bool LibraryTreeWidget::setSubModelsFileNameOneFileHelper(LibraryTreeNode *pLibraryTreeNode, QString filePath)
{
  /* if user has done some changes in the Modelica text view then save & validate it in the AST before saving it to file. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return false;
  }
  for (int i = 0 ; i < pLibraryTreeNode->childCount(); i++)
  {
    LibraryTreeNode *pChildLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(pLibraryTreeNode->child(i));
    if (pChildLibraryTreeNode->childCount() > 0)
    {
      if (!setSubModelsFileNameOneFileHelper(pChildLibraryTreeNode, filePath))
        return false;
    }
    else
    {
      /* if user has done some changes in the Modelica text view then save & validate it in the AST before saving it to file. */
      if (pChildLibraryTreeNode->getModelWidget())
      {
        if (!pChildLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
          return false;
      }
    }
    mpMainWindow->getStatusBar()->showMessage(QString(tr("Saving")).append(" ").append(pChildLibraryTreeNode->getNameStructure()));
    mpMainWindow->getOMCProxy()->setSourceFile(pChildLibraryTreeNode->getNameStructure(), filePath);
    pChildLibraryTreeNode->setFileName(filePath);
  }
  return true;
}

void LibraryTreeWidget::setSubModelsSavedOneFileHelper(LibraryTreeNode *pLibraryTreeNode)
{
  for (int i = 0 ; i < pLibraryTreeNode->childCount(); i++)
  {
    LibraryTreeNode *pChildLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(pLibraryTreeNode->child(i));
    if (pChildLibraryTreeNode->childCount() > 0)
      setSubModelsSavedOneFileHelper(pChildLibraryTreeNode);
    pChildLibraryTreeNode->setIsSaved(true);
    if (pChildLibraryTreeNode->getModelWidget())
    {
      pChildLibraryTreeNode->getModelWidget()->setWindowTitle(pChildLibraryTreeNode->getNameStructure());
      pChildLibraryTreeNode->getModelWidget()->setModelFilePathLabel(pChildLibraryTreeNode->getFileName());
    }
  }
}

bool LibraryTreeWidget::saveLibraryTreeNodeFolderHelper(LibraryTreeNode *pLibraryTreeNode)
{
  mpMainWindow->getStatusBar()->showMessage(QString(tr("Saving")).append(" ").append(pLibraryTreeNode->getNameStructure()));
  QString directoryName;
  QString fileName;
  if (pLibraryTreeNode->getFileName().isEmpty())
  {
    directoryName = StringHandler::getExistingDirectory(this, QString(Helper::applicationName).append(" - ").append(Helper::chooseDirectory), NULL);
    if (directoryName.isEmpty())   // if user press ESC
      return false;
    directoryName = directoryName.replace("\\", "/");
    // set the fileName for the model.
    fileName = QString(directoryName).append("/package.mo");
  }
  else
  {
    fileName = pLibraryTreeNode->getFileName();
    QFileInfo fileInfo(fileName);
    directoryName = fileInfo.absoluteDir().absolutePath();
  }
  /* if user has done some changes in the Modelica text view then save & validate it in the AST before saving it to file. */
  if (pLibraryTreeNode->getModelWidget())
  {
    if (!pLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
      return false;
  }
  mpMainWindow->getOMCProxy()->setSourceFile(pLibraryTreeNode->getNameStructure(), fileName);
  // save the model through OMC
  if (mpMainWindow->getOMCProxy()->save(pLibraryTreeNode->getNameStructure(), fileName))
  {
    pLibraryTreeNode->setIsSaved(true);
    pLibraryTreeNode->setFileName(fileName);
    if (pLibraryTreeNode->getModelWidget())
    {
      pLibraryTreeNode->getModelWidget()->setWindowTitle(pLibraryTreeNode->getNameStructure());
      pLibraryTreeNode->getModelWidget()->setModelFilePathLabel(fileName);
    }
    if (!saveSubModelsFolderHelper(pLibraryTreeNode, directoryName))
      return false;
    return true;
  }
  else
  {
    mpMainWindow->getOMCProxy()->printMessagesStringInternal();
    return false;
  }
}

bool LibraryTreeWidget::saveSubModelsFolderHelper(LibraryTreeNode *pLibraryTreeNode, QString directoryName)
{
  for (int i = 0 ; i < pLibraryTreeNode->childCount(); i++)
  {
    LibraryTreeNode *pChildLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(pLibraryTreeNode->child(i));
    /* if user has done some changes in the Modelica text view then save & validate it in the AST before saving it to file. */
    if (pChildLibraryTreeNode->getModelWidget())
    {
      if (!pChildLibraryTreeNode->getModelWidget()->getModelicaTextEditor()->validateModelicaText())
        return false;
    }
    QString directory;
    if (pChildLibraryTreeNode->getModelicaType() != StringHandler::Package)
    {
      directory = directoryName;
      mpMainWindow->getStatusBar()->showMessage(QString(tr("Saving")).append(" ").append(pChildLibraryTreeNode->getNameStructure()));
      QString fileName = QString(directory).append("/").append(pChildLibraryTreeNode->getName()).append(".mo");
      mpMainWindow->getOMCProxy()->setSourceFile(pChildLibraryTreeNode->getNameStructure(), fileName);
      if (mpMainWindow->getOMCProxy()->save(pChildLibraryTreeNode->getNameStructure(), fileName))
      {
        pChildLibraryTreeNode->setIsSaved(true);
        pChildLibraryTreeNode->setFileName(fileName);
        if (pChildLibraryTreeNode->getModelWidget())
        {
          pChildLibraryTreeNode->getModelWidget()->setWindowTitle(pChildLibraryTreeNode->getNameStructure());
          pChildLibraryTreeNode->getModelWidget()->setModelFilePathLabel(fileName);
        }
      }
      else
      {
        mpMainWindow->getOMCProxy()->printMessagesStringInternal();
        return false;
      }
    }
    else
    {
      directory = QString(directoryName).append("/").append(pChildLibraryTreeNode->getName());
      if (!QDir().exists(directory))
        QDir().mkpath(directory);
      if (QDir().exists(directory))
      {
        mpMainWindow->getStatusBar()->showMessage(QString(tr("Saving")).append(" ").append(pChildLibraryTreeNode->getNameStructure()));
        QString fileName = QString(directory).append("/package.mo");
        mpMainWindow->getOMCProxy()->setSourceFile(pChildLibraryTreeNode->getNameStructure(), fileName);
        if (mpMainWindow->getOMCProxy()->save(pChildLibraryTreeNode->getNameStructure(), fileName))
        {
          pChildLibraryTreeNode->setIsSaved(true);
          pChildLibraryTreeNode->setFileName(fileName);
          if (pChildLibraryTreeNode->getModelWidget())
          {
            pChildLibraryTreeNode->getModelWidget()->setWindowTitle(pChildLibraryTreeNode->getNameStructure());
            pChildLibraryTreeNode->getModelWidget()->setModelFilePathLabel(fileName);
          }
        }
        else
        {
          mpMainWindow->getOMCProxy()->printMessagesStringInternal();
          return false;
        }
      }
    }
    if (pChildLibraryTreeNode->childCount() > 0)
      saveSubModelsFolderHelper(pChildLibraryTreeNode, directory);
  }
  return true;
}

bool LibraryTreeWidget::saveLibraryTreeNodeOneFileOrFolderHelper(LibraryTreeNode *pLibraryTreeNode)
{
  QFileInfo fileInfo(pLibraryTreeNode->getFileName());
  /* if library is folder structure */
  if (fileInfo.fileName().compare("package.mo") == 0)
    return saveLibraryTreeNodeFolderHelper(pLibraryTreeNode);
  else
    return saveLibraryTreeNodeOneFileHelper(pLibraryTreeNode);
}

/*!
  Deletes the LibraryTreeNode.
  Deletes the ModelWidget of LibraryTreeNode and remove the QMdiSubWindow from MdiArea and delete it.
  */
void LibraryTreeWidget::unloadLibraryTreeNodeAndModelWidget(LibraryTreeNode *pLibraryTreeNode)
{
  if (pLibraryTreeNode->getModelWidget())
  {
    QMdiSubWindow *pMdiSubWindow = mpMainWindow->getModelWidgetContainer()->getMdiSubWindow(pLibraryTreeNode->getModelWidget());
    if (pMdiSubWindow)
    {
      pMdiSubWindow->close();
      pMdiSubWindow->deleteLater();
    }
    pLibraryTreeNode->getModelWidget()->deleteLater();
  }
  /* delete the complete LibraryTreeNode */
  qDeleteAll(pLibraryTreeNode->takeChildren());
  mLibraryTreeNodesList.removeOne(pLibraryTreeNode);
  mExpandedLibraryTreeNodesList.removeOne(pLibraryTreeNode);
  /* Update the model switcher toolbar button. */
  mpMainWindow->updateModelSwitcherMenu(0);
  delete pLibraryTreeNode;
}

//! Makes a library expand.
//! @param item is the library to show.
void LibraryTreeWidget::expandLibraryTreeNode(QTreeWidgetItem *item)
{
  blockSignals(true);
  collapseItem(item);
  blockSignals(false);
  if (!isLibraryTreeNodeExpanded(item))
  {
    LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(item);
    addToExpandedLibraryTreeNodesList(pLibraryTreeNode);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    expandLibraryTreeNode(pLibraryTreeNode);
    mpMainWindow->getStatusBar()->clearMessage();
    QApplication::restoreOverrideCursor();
  }
  blockSignals(true);
  expandItem(item);
  blockSignals(false);
}

void LibraryTreeWidget::showContextMenu(QPoint point)
{
  int adjust = 24;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(itemAt(point));
  if (pLibraryTreeNode)
  {
    QMenu menu(this);
    switch (pLibraryTreeNode->getLibraryType())
    {
      case LibraryTreeNode::Modelica:
      default:
        menu.addAction(mpViewClassAction);
        menu.addAction(mpViewDocumentationAction);
        if (!(pLibraryTreeNode->isSystemLibrary() || isSearchedTree()))
        {
          menu.addSeparator();
          menu.addAction(mpNewModelicaClassAction);
        }
        menu.addSeparator();
        menu.addAction(mpInstantiateModelAction);
        menu.addAction(mpCheckModelAction);
        menu.addAction(mpCheckAllModelsAction);
        menu.addAction(mpSimulateAction);
        menu.addAction(mpSimulationSetupAction);
        /* If item is OpenModelica or part of it or is search tree item then don't show the unload for it. */
        if (!((StringHandler::getFirstWordBeforeDot(pLibraryTreeNode->getNameStructure()).compare("OpenModelica") == 0)  || isSearchedTree()))
        {
          menu.addSeparator();
          menu.addAction(mpUnloadClassAction);
          /* Only used for development testing. */
          /*menu.addAction(mpRefreshAction);*/
        }
        menu.addSeparator();
        menu.addAction(mpExportFMUAction);
        menu.addAction(mpExportXMLAction);
        menu.addAction(mpExportFigaroAction);
        break;
      case LibraryTreeNode::Text:
        menu.addAction(mpUnloadTextFileAction);
        break;
      case LibraryTreeNode::TLM:
        menu.addAction(mpUnloadXMLFileAction);
        break;
    }
    point.setY(point.y() + adjust);
    menu.exec(mapToGlobal(point));
  }
}

void LibraryTreeWidget::createNewModelicaClass()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
  {
    ModelicaClassDialog *pModelicaClassDialog = new ModelicaClassDialog(mpMainWindow);
    pModelicaClassDialog->getParentClassTextBox()->setText(pLibraryTreeNode->getNameStructure());
    pModelicaClassDialog->show();
  }
}

void LibraryTreeWidget::viewDocumentation()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
  {
    mpMainWindow->getDocumentationWidget()->showDocumentation(pLibraryTreeNode->getNameStructure());
    mpMainWindow->getDocumentationDockWidget()->show();
  }
}

void LibraryTreeWidget::simulate()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    mpMainWindow->simulate(pLibraryTreeNode);
}

void LibraryTreeWidget::simulationSetup()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    mpMainWindow->simulationSetup(pLibraryTreeNode);
}

void LibraryTreeWidget::instantiateModel()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    mpMainWindow->instantiatesModel(pLibraryTreeNode);
}

void LibraryTreeWidget::checkModel()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    mpMainWindow->checkModel(pLibraryTreeNode);
}

void LibraryTreeWidget::checkAllModels()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    mpMainWindow->checkAllModels(pLibraryTreeNode);
}

void LibraryTreeWidget::unloadClass()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    unloadClass(pLibraryTreeNode);
}

void LibraryTreeWidget::unloadTextFile()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    unloadTextFile(pLibraryTreeNode);
}

void LibraryTreeWidget::unloadXMLFile()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    unloadXMLFile(pLibraryTreeNode);
}

void LibraryTreeWidget::refresh()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    if (pLibraryTreeNode->getModelWidget())
      pLibraryTreeNode->getModelWidget()->refresh();
}

void LibraryTreeWidget::exportModelFMU()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    mpMainWindow->exportModelFMU(pLibraryTreeNode);
}

void LibraryTreeWidget::exportModelXML()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    mpMainWindow->exportModelXML(pLibraryTreeNode);
}

void LibraryTreeWidget::exportModelFigaro()
{
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (selectedItemsList.isEmpty())
    return;
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  if (pLibraryTreeNode)
    mpMainWindow->exportModelFigaro(pLibraryTreeNode);
}

void LibraryTreeWidget::openFile(QString fileName, QString encoding, bool showProgress, bool checkFileExists)
{
  /* if the file doesn't exist then remove it from the recent files list. */
  if (checkFileExists)
  {
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists())
    {
      QMessageBox::information(mpMainWindow, QString(Helper::applicationName).append(" - ").append(Helper::information),
                               GUIMessages::getMessage(GUIMessages::FILE_NOT_FOUND).arg(fileName), Helper::ok);
      QSettings *pSettings = OpenModelica::getApplicationSettings();
      QList<QVariant> files = pSettings->value("recentFilesList/files").toList();
      // remove the RecentFile instance from the list.
      foreach (QVariant file, files)
      {
        RecentFile recentFile = qvariant_cast<RecentFile>(file);
        if (recentFile.fileName.compare(fileName) == 0)
          files.removeOne(file);
      }
      pSettings->setValue("recentFilesList/files", files);
      mpMainWindow->updateRecentFileActions();
      return;
    }
  }
  // get the class names now to check if they are already loaded or not
  QStringList existingmodelsList;
  if (showProgress) mpMainWindow->getStatusBar()->showMessage(QString(Helper::loading).append(": ").append(fileName));
  if (mpMainWindow->getOMCProxy()->parseFile(fileName, encoding))
  {
    QString result = StringHandler::removeFirstLastCurlBrackets(mpMainWindow->getOMCProxy()->getResult());
    QStringList modelsList = result.split(",", QString::SkipEmptyParts);
    /*
      Only allow loading of files that has just one nonstructured entity.
      From Modelica specs section 13.2.2.2,
      "A nonstructured entity [e.g. the file A.mo] shall contain only a stored-definition that defines a class [A] with a name
       matching the name of the nonstructured entity."
      */
    if (modelsList.size() > 1)
    {
      QMessageBox *pMessageBox = new QMessageBox(mpMainWindow);
      pMessageBox->setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::error));
      pMessageBox->setIcon(QMessageBox::Critical);
      pMessageBox->setText(QString(GUIMessages::getMessage(GUIMessages::UNABLE_TO_LOAD_FILE).arg(fileName)));
      pMessageBox->setInformativeText(QString(GUIMessages::getMessage(GUIMessages::MULTIPLE_TOP_LEVEL_CLASSES)).arg(fileName)
                                      .arg(modelsList.join(",")));
      pMessageBox->setStandardButtons(QMessageBox::Ok);
      pMessageBox->exec();
      return;
    }
    bool existModel = false;
    // check if the model already exists
    foreach(QString model, modelsList)
    {
      if (mpMainWindow->getOMCProxy()->existClass(model))
      {
        existingmodelsList.append(model);
        existModel = true;
      }
    }
    // if existModel is true, show user an error message
    if (existModel)
    {
      QMessageBox *pMessageBox = new QMessageBox(mpMainWindow);
      pMessageBox->setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::information));
      pMessageBox->setIcon(QMessageBox::Information);
      pMessageBox->setText(QString(GUIMessages::getMessage(GUIMessages::UNABLE_TO_LOAD_FILE).arg(fileName)));
      pMessageBox->setInformativeText(QString(GUIMessages::getMessage(GUIMessages::REDEFINING_EXISTING_CLASSES))
                                      .arg(existingmodelsList.join(",")).append("\n")
                                      .append(GUIMessages::getMessage(GUIMessages::DELETE_AND_LOAD).arg(fileName)));
      pMessageBox->setStandardButtons(QMessageBox::Ok);
      pMessageBox->exec();
    }
    // if no conflicting model found then just load the file simply
    else
    {
      // load the file in OMC
      if (mpMainWindow->getOMCProxy()->loadFile(fileName, encoding))
      {
        // create library tree nodes for loaded models
        int progressvalue = 0;
        if (showProgress)
        {
          mpMainWindow->getProgressBar()->setRange(0, modelsList.size());
          mpMainWindow->showProgressBar();
        }
        foreach (QString model, modelsList)
        {
          LibraryTreeNode *pLibraryTreeNode = addLibraryTreeNode(model, mpMainWindow->getOMCProxy()->getClassRestriction(model), "");
          createLibraryTreeNodes(pLibraryTreeNode);
          if (showProgress) mpMainWindow->getProgressBar()->setValue(++progressvalue);
        }
        mpMainWindow->addRecentFile(fileName, encoding);
        if (showProgress) mpMainWindow->hideProgressBar();
      }
    }
  }
  if (showProgress) mpMainWindow->getStatusBar()->clearMessage();
}

void LibraryTreeWidget::parseAndLoadModelicaText(QString modelText)
{
  QStringList modelsList = mpMainWindow->getOMCProxy()->parseString(StringHandler::escapeString(modelText));
  if (modelsList.size() == 0)
    return;
  QStringList existingmodelsList;
  bool existModel = false;
  // check if the model already exists
  foreach(QString model, modelsList)
  {
    if (mpMainWindow->getOMCProxy()->existClass(model))
    {
      existingmodelsList.append(model);
      existModel = true;
    }
  }
  // check if existModel is true
  if (existModel)
  {
    QMessageBox *pMessageBox = new QMessageBox(mpMainWindow);
    pMessageBox->setWindowTitle(QString(Helper::applicationName).append(" - ").append(Helper::information));
    pMessageBox->setIcon(QMessageBox::Information);
    pMessageBox->setText(QString(GUIMessages::getMessage(GUIMessages::UNABLE_TO_LOAD_MODEL).arg("")));
    pMessageBox->setInformativeText(QString(GUIMessages::getMessage(GUIMessages::REDEFINING_EXISTING_CLASSES))
                                    .arg(existingmodelsList.join(",")).append("\n")
                                    .append(GUIMessages::getMessage(GUIMessages::DELETE_AND_LOAD).arg("")));
    pMessageBox->setStandardButtons(QMessageBox::Ok);
    pMessageBox->exec();
  }
  // if no conflicting model found then just load the file simply
  else
  {
    // load the model text in OMC
    if (mpMainWindow->getOMCProxy()->loadString(StringHandler::escapeString(modelText)))
    {
      foreach (QString model, modelsList)
      {
        QString modelName = StringHandler::getLastWordAfterDot(model);
        QString parentName = StringHandler::removeLastWordAfterDot(model);
        if (modelName.compare(parentName) == 0)
          parentName = "";
        LibraryTreeNode *pLibraryTreeNode;
        pLibraryTreeNode = addLibraryTreeNode(modelName, mpMainWindow->getOMCProxy()->getClassRestriction(modelName), parentName);
        createLibraryTreeNodes(pLibraryTreeNode);
      }
    }
  }
}

void LibraryTreeWidget::showModelWidget(LibraryTreeNode *pLibraryTreeNode, bool newClass, bool extendsClass, QString text)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  QList<QTreeWidgetItem*> selectedItemsList = selectedItems();
  if (pLibraryTreeNode == 0)
  {
    if (selectedItemsList.isEmpty())
    {
      QApplication::restoreOverrideCursor();
      return;
    }
    pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(selectedItemsList.at(0));
  }
  mpMainWindow->getPerspectiveTabBar()->setCurrentIndex(1);
  /* Search Tree Items never have model widget so find the equivalent Library Tree Node */
  if (isSearchedTree())
  {
    pLibraryTreeNode = mpMainWindow->getLibraryTreeWidget()->getLibraryTreeNode(pLibraryTreeNode->getNameStructure());
    mpMainWindow->getLibraryTreeWidget()->showModelWidget(pLibraryTreeNode, newClass, extendsClass);
    QApplication::restoreOverrideCursor();
    return;
  }
  if (pLibraryTreeNode->getModelWidget())
  {
    pLibraryTreeNode->getModelWidget()->setWindowTitle(pLibraryTreeNode->getNameStructure() + (pLibraryTreeNode->isSaved() ? "" : "*"));
    mpMainWindow->getModelWidgetContainer()->addModelWidget(pLibraryTreeNode->getModelWidget());
  }
  else
  {
    ModelWidget *pModelWidget;
    if (pLibraryTreeNode->getLibraryType() == LibraryTreeNode::Modelica)
      pModelWidget = new ModelWidget(newClass, extendsClass, pLibraryTreeNode, mpMainWindow->getModelWidgetContainer());
    else if (pLibraryTreeNode->getLibraryType() == LibraryTreeNode::Text)
        pModelWidget = new ModelWidget(text, pLibraryTreeNode, mpMainWindow->getModelWidgetContainer());
    else if (pLibraryTreeNode->getLibraryType() == LibraryTreeNode::TLM)
      pModelWidget = new ModelWidget(text, text, pLibraryTreeNode, mpMainWindow->getModelWidgetContainer());
    pLibraryTreeNode->setModelWidget(pModelWidget);
    pLibraryTreeNode->getModelWidget()->setWindowTitle(pLibraryTreeNode->getNameStructure() + (pLibraryTreeNode->isSaved() ? "" : "*"));
    mpMainWindow->getModelWidgetContainer()->addModelWidget(pModelWidget);
  }
  QApplication::restoreOverrideCursor();
}

void LibraryTreeWidget::openLibraryTreeNode(QString nameStructure)
{
  LibraryTreeNode *pLibraryTreeNode = getLibraryTreeNode(nameStructure);
  if (!pLibraryTreeNode)
    return;
  showModelWidget(pLibraryTreeNode);
}

void LibraryTreeWidget::loadLibraryComponent(LibraryTreeNode *pLibraryTreeNode)
{
  OMCProxy *pOMCProxy = mpMainWindow->getOMCProxy();
  QString result = pOMCProxy->getIconAnnotation(pLibraryTreeNode->getNameStructure());
  LibraryComponent *libComponent = new LibraryComponent(result, pLibraryTreeNode->getNameStructure(), pOMCProxy);
  QPixmap pixmap = libComponent->getComponentPixmap(Helper::iconSize);
  // if the component does not have icon annotation check if it has non standard dymola annotation or not.
  if (pixmap.isNull())
  {
    pOMCProxy->sendCommand("getNamedAnnotation(" + pLibraryTreeNode->getNameStructure() + ", __Dymola_DocumentationClass)");
    if (StringHandler::unparseBool(StringHandler::removeFirstLastCurlBrackets(pOMCProxy->getResult())) || pLibraryTreeNode->isDocumentationClass())
    {
      result = pOMCProxy->getIconAnnotation("ModelicaReference.Icons.Information");
      libComponent = new LibraryComponent(result, pLibraryTreeNode->getNameStructure(), pOMCProxy);
      pixmap = libComponent->getComponentPixmap(Helper::iconSize);
      // if still the pixmap is null for some unknown reasons then used the pre defined image
      if (pixmap.isNull())
        pLibraryTreeNode->setIcon(0, QIcon(":/Resources/icons/info-icon.png"));
      else
        pLibraryTreeNode->setIcon(0, QIcon(pixmap));
    }
    // if the component does not have non standard dymola annotation as well.
    else
      pLibraryTreeNode->setIcon(0, pLibraryTreeNode->getModelicaNodeIcon());
  }
  else
  {
    pLibraryTreeNode->setIcon(0, QIcon(pixmap));
  }
  addLibraryComponentObject(libComponent);
}

void LibraryTreeWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (!itemAt(event->pos()))
    return;
  showModelWidget();
  QTreeWidget::mouseDoubleClickEvent(event);
}

void LibraryTreeWidget::startDrag(Qt::DropActions supportedActions)
{
  LibraryTreeNode *pLibraryTreeNode = dynamic_cast<LibraryTreeNode*>(currentItem());
  // get the component pixmap to show on drag
  LibraryComponent *pLibraryComponent = getLibraryComponentObject(pLibraryTreeNode->getNameStructure());
  if (isSearchedTree())
    pLibraryTreeNode = mpMainWindow->getLibraryTreeWidget()->getLibraryTreeNode(pLibraryTreeNode->getNameStructure());
  QByteArray itemData;
  QDataStream dataStream(&itemData, QIODevice::WriteOnly);
  dataStream << pLibraryTreeNode->getNameStructure();
  QMimeData *mimeData = new QMimeData;
  mimeData->setData(Helper::modelicaComponentFormat, itemData);
  qreal adjust = 35;
  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  // if we have component pixmap
  if (pLibraryComponent)
  {
    QPixmap pixmap = pLibraryComponent->getComponentPixmap(QSize(50, 50));
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint((drag->hotSpot().x() + adjust), (drag->hotSpot().y() + adjust)));
  }
  drag->exec(supportedActions);
}

Qt::DropActions LibraryTreeWidget::supportedDropActions() const
{
  return Qt::CopyAction;
}

LibraryComponent::LibraryComponent(QString value, QString className, OMCProxy *omc)
{
  mClassName = className;
  mpComponent = new Component(value, className, omc);

  if (mpComponent->boundingRect().width() > 1)
    mRectangle = mpComponent->boundingRect();
  else
    mRectangle = QRectF(-100.0, -100.0, 200.0, 200.0);

  qreal adjust = 25;
  mRectangle.setX(mRectangle.x() - adjust);
  mRectangle.setY(mRectangle.y() - adjust);
  mRectangle.setWidth(mRectangle.width() + adjust);
  mRectangle.setHeight(mRectangle.height() + adjust);

  mpGraphicsView = new QGraphicsView;
  mpGraphicsView->setScene(new QGraphicsScene);
  mpGraphicsView->setSceneRect(mRectangle);
  mpGraphicsView->scene()->addItem(mpComponent);
}

LibraryComponent::~LibraryComponent()
{
  delete mpComponent;
  delete mpGraphicsView;
}

QPixmap LibraryComponent::getComponentPixmap(QSize size)
{
  // if view is empty we return null QPixmap
  mHasIconAnnotation = false;
  hasIconAnnotation(mpComponent);
  if (!mHasIconAnnotation)
    return QPixmap();

  QPixmap pixmap(size);
  pixmap.fill(QColor(Qt::transparent));
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  painter.setWindow(mRectangle.toRect());
  painter.scale(1.0, -1.0);
  mpGraphicsView->scene()->render(&painter, mRectangle, mpGraphicsView->sceneRect());
  painter.end();
  return pixmap;
}

void LibraryComponent::hasIconAnnotation(Component *pComponent)
{
  if (!pComponent->getShapesList().isEmpty())
  {
    mHasIconAnnotation = true;
  }
  else
  {
    foreach (Component *inheritedComponent, pComponent->getInheritanceList())
    {
      hasIconAnnotation(inheritedComponent);
    }
    foreach (Component *childComponent, pComponent->getComponentsList())
    {
      hasIconAnnotation(childComponent);
    }
  }
}
