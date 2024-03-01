#ifndef ANALYSISFORMBASE_H
#define ANALYSISFORMBASE_H

#include <QQuickItem>
#include "utils.h"

class AnalysisBase;

class AnalysisFormBase : public QQuickItem
{
	Q_OBJECT

public:

	explicit AnalysisFormBase(QQuickItem *parent = nullptr) : QQuickItem(parent)	{}

	virtual stringset	usedVariables()												{ return stringset(); }
	virtual void		cleanUpForm()												{}
	virtual bool		hasError()													{ return false; }
	virtual bool		runOnChange()												{ return false; }
	virtual void		setMustBe(		std::set<std::string>						mustBe)			{}
	virtual void		setMustContain(	std::map<std::string,std::set<std::string>> mustContain)	{}
	virtual void		setHasVolatileNotes(bool hasVolatileNotes)					{}
	virtual bool		formCompleted()										const	{ return false;	}
	virtual Q_INVOKABLE bool initialized()									const	{ return false; }
	virtual QString		generateRSyntax(bool useHtml = false)				const	{ return QString(); }

	const QString		rSyntaxControlName = "__RSyntaxTextArea";

public slots:
	virtual void	setAnalysis(AnalysisBase * analysis)							{}
	virtual void	setShowRButton(bool showRButton)								{}
	virtual void	setDeveloperMode(bool developerMode)							{}
	virtual void	runScriptRequestDone(const QString& result, const QString& requestId, bool hasError) {}


signals:
	void	rSourceChanged(const QString& name);
	void	refreshTableViewModels();
	void	needsRefreshChanged();
	void	titleChanged();
	void	languageChanged();
};

#endif // ANALYSISFORMBASE_H
