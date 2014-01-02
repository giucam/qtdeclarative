/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4ARRAYDATA_H
#define QV4ARRAYDATA_H

#include "qv4global_p.h"
#include "qv4managed_p.h"
#include "qv4property_p.h"
#include "qv4sparsearray_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct ArrayData;

struct ArrayVTable
{
    uint type;
    void (*destroy)(ArrayData *d);
    void (*markObjects)(ArrayData *, ExecutionEngine *e);
    void (*reserve)(ArrayData *d, uint n);
    ReturnedValue (*get)(const ArrayData *d, uint index);
    bool (*put)(ArrayData *d, uint index, ValueRef value);
    bool (*putArray)(ArrayData *d, uint index, SafeValue *values, uint n);
    bool (*del)(ArrayData *d, uint index);
    void (*setAttribute)(ArrayData *d, uint index, PropertyAttributes attrs);
    PropertyAttributes (*attribute)(const ArrayData *d, uint index);
    void (*push_front)(ArrayData *d, SafeValue *values, uint n);
    ReturnedValue (*pop_front)(ArrayData *d);
    uint (*truncate)(ArrayData *d, uint newLen);
    uint (*length)(const ArrayData *d);
};


struct Q_QML_EXPORT ArrayData
{
    ArrayData()
        : vtable(0)
        , alloc(0)
        , type(0)
        , attrs(0)
        , data(0)
    {
    }

    enum Type {
        Simple = 0,
        Complex = 1,
        Sparse = 2,
        Custom = 3
    };

    const ArrayVTable *vtable;
    uint alloc;
    uint type;
    PropertyAttributes *attrs;
    SafeValue *data;

    bool isSparse() const { return this && type == Sparse; }

    uint length() const {
        if (!this)
            return 0;
        return vtable->length(this);
    }

    bool hasAttributes() const {
        return this && attrs;
    }
    void ensureAttributes();
    PropertyAttributes attributes(int i) const {
        Q_ASSERT(this);
        return attrs ? vtable->attribute(this, i) : Attr_Data;
    }
    void setAttributes(uint i, PropertyAttributes a) {
        Q_ASSERT(this);
        if (attrs || a != Attr_Data) {
            ensureAttributes();
            a.resolve();
            vtable->setAttribute(this, i, a);
        }
    }

    bool isEmpty(uint i) const {
        if (!this)
            return true;
        return (vtable->get(this, i) == Primitive::emptyValue().asReturnedValue());
    }


    inline void destroy() {
        vtable->destroy(this);
    }

    inline void markObjects(ExecutionEngine *e) {
        vtable->markObjects(this, e);
    }

    inline void push_front(SafeValue *values, uint nValues) {
        vtable->push_front(this, values, nValues);
    }
    inline ReturnedValue pop_front() {
        return vtable->pop_front(this);
    }
    inline uint push_back(uint l, uint n, SafeValue *values) {
        vtable->putArray(this, l, values, n);
        return length();
    }
    inline bool deleteIndex(uint index) {
        return vtable->del(this, index);
    }
    inline uint truncate(uint newLen) {
        if (!this)
            return newLen;
        return vtable->truncate(this, newLen);
    }
    bool put(uint index, ValueRef value) {
        return vtable->put(this, index, value);
    }
    bool put(uint index, SafeValue *values, uint n) {
        return vtable->putArray(this, index, values, n);
    }
    ReturnedValue get(uint i) const {
        if (!this)
            return Primitive::emptyValue().asReturnedValue();
        return vtable->get(this, i);
    }
    inline Property *getProperty(uint index) const;

    static void sort(ExecutionContext *context, ObjectRef thisObject, const ValueRef comparefn, uint dataLen);
    static uint append(Object *obj, const ArrayObject *otherObj, uint n);
    static Property *insert(Object *o, uint index, bool isAccessor = false);
};

struct Q_QML_EXPORT SimpleArrayData : public ArrayData
{
    SimpleArrayData()
        : ArrayData()
        , len(0)
        , offset(0)
    { vtable = &static_vtbl; }

    uint len;
    uint offset;

    static void getHeadRoom(ArrayData *d);
    static void reserve(ArrayData *d, uint n);

    static void destroy(ArrayData *d);
    static void markObjects(ArrayData *d, ExecutionEngine *e);

    static ReturnedValue get(const ArrayData *d, uint index);
    static bool put(ArrayData *d, uint index, ValueRef value);
    static bool putArray(ArrayData *d, uint index, SafeValue *values, uint n);
    static bool del(ArrayData *d, uint index);
    static void setAttribute(ArrayData *d, uint index, PropertyAttributes attrs);
    static PropertyAttributes attribute(const ArrayData *d, uint index);
    static void push_front(ArrayData *d, SafeValue *values, uint n);
    static ReturnedValue pop_front(ArrayData *d);
    static uint truncate(ArrayData *d, uint newLen);
    static uint length(const ArrayData *d);

    static const ArrayVTable static_vtbl;

};

struct Q_QML_EXPORT SparseArrayData : public ArrayData
{
    SparseArrayData()
        : freeList(0)
        , sparse(0)
    { vtable = &static_vtbl; }

    uint freeList;
    SparseArray *sparse;

    static uint allocate(ArrayData *d, bool doubleSlot = false);
    static void free(ArrayData *d, uint idx);

    static void destroy(ArrayData *d);
    static void markObjects(ArrayData *d, ExecutionEngine *e);

    static void reserve(ArrayData *d, uint n);
    static ReturnedValue get(const ArrayData *d, uint index);
    static bool put(ArrayData *d, uint index, ValueRef value);
    static bool putArray(ArrayData *d, uint index, SafeValue *values, uint n);
    static bool del(ArrayData *d, uint index);
    static void setAttribute(ArrayData *d, uint index, PropertyAttributes attrs);
    static PropertyAttributes attribute(const ArrayData *d, uint index);
    static void push_front(ArrayData *d, SafeValue *values, uint n);
    static ReturnedValue pop_front(ArrayData *d);
    static uint truncate(ArrayData *d, uint newLen);
    static uint length(const ArrayData *d);

    static const ArrayVTable static_vtbl;
};


inline Property *ArrayData::getProperty(uint index) const
{
    if (!this)
        return 0;
    if (type != Sparse) {
        const SimpleArrayData *that = static_cast<const SimpleArrayData *>(this);
        if (index >= that->len || data[index].isEmpty())
            return 0;
        return reinterpret_cast<Property *>(data + index);
    } else {
        SparseArrayNode *n = static_cast<const SparseArrayData *>(this)->sparse->findNode(index);
        if (!n)
            return 0;
        return reinterpret_cast<Property *>(data + n->value);
    }
}

}

QT_END_NAMESPACE

#endif