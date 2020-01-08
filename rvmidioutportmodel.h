#ifndef RVMIDIOUTPORTMODEL_H
#define RVMIDIOUTPORTMODEL_H

#include <QAbstractItemModel>

#include "rvmidiportinfo.h"

class RvMidi;

class RvMidiOutPortModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit RvMidiOutPortModel(RvMidi *rvmidi, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &) const override { return 1; }

    Qt::ItemFlags flags( const QModelIndex &index) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override { Q_UNUSED(parent); return createIndex( row, column); }
    QModelIndex parent(const QModelIndex &) const override { return QModelIndex(); }

private slots:
    void readPortList();

private:
    mutable QList<RvMidiPortInfo> portList;
    RvMidi *rvmidi;
};

#endif // RVMIDIOUTPORTMODEL_H
