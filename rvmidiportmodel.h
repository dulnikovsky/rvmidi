#ifndef RVMIDIPORTMODEL_H
#define RVMIDIPORTMODEL_H

#include <QAbstractItemModel>

#include "rvmidiportinfo.h"

class RvMidi;

class RvMidiPortModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Direction{ ReadablePorts, WritablePorts };

    explicit RvMidiPortModel(RvMidi &rvmidi, Direction d, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &) const override { return 1; }

    Qt::ItemFlags flags( const QModelIndex &index) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override { Q_UNUSED(parent); return createIndex( row, column); }
    QModelIndex parent(const QModelIndex &) const override { return QModelIndex(); }

private:
    Direction direction;
    mutable QList<RvMidiPortInfo> portList;
    RvMidi &rvmidi;
};

#endif // RVMIDIPORTMODEL_H
