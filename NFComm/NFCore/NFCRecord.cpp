// -------------------------------------------------------------------------
//    @FileName         :    NFCRecord.h
//    @Author           :    LvSheng.Huang
//    @Date             :    2012-03-01
//    @Module           :    NFCRecord
//
// -------------------------------------------------------------------------

#include <exception>
#include "NFCRecord.h"

NFCRecord::NFCRecord()
{
    mSelf = 0;
    mbSave = false;
    mbPublic = false;
    mbPrivate = false;
    mbHasKey = false;
    mstrRecordName = "";
    mnMaxRow = 0;

}

NFCRecord::NFCRecord(const NFIDENTID& self, const std::string& strRecordName, const NFIValueList& valueList, const NFIValueList& keyList, const NFIValueList& descList, const NFIValueList& tagList, int nMaxRow, bool bPublic,  bool bPrivate,  bool bSave, int nIndex)
{
    mVarRecordType = valueList;
    mVarRecordDesc = descList;
    mVarRecordKey = keyList;
    mSelf = self;
    mbSave = bSave;
    mbPublic = bPublic;
    mbPrivate = bPrivate;
    mnIndex = nIndex;

    mstrRecordName = strRecordName;

    mnMaxRow = nMaxRow;
    //确定大小
    mVecUsedState.resize(mnMaxRow);

    for (int i = 0; i < mnMaxRow; i++)
    {
        mVecUsedState[i] = 0;
    }

    for (int i = 0; i < keyList.GetCount(); ++i)
    {
        if (keyList.IntVal(i) > 0)
        {
            mbHasKey = true;
            break;
        }
    }

    for (int i = 0; i < GetRows() * GetCols(); i++)
    {
        mtRecordVec.push_back(std::shared_ptr<NFIValueList::VarData>());
    }
}

NFCRecord::~NFCRecord()
{
    for (auto iter = mtRecordVec.begin(); iter != mtRecordVec.end(); ++iter)
    {
        iter->reset();
    }
    
    for (auto iter = mtRecordCallback.begin(); iter != mtRecordCallback.end(); ++iter)
    {
        iter->reset();
    }

    mtRecordVec.clear();
    mVecUsedState.clear();
    mtRecordCallback.clear();
}

int NFCRecord::GetCols() const
{
    return mVarRecordType.GetCount();
}

int NFCRecord::GetRows() const
{
    return mnMaxRow;
}

VARIANT_TYPE NFCRecord::GetColType(const int nCol) const
{
    return mVarRecordType.Type(nCol);
}

const std::string& NFCRecord::GetColTag( const int nCol ) const
{
    return mVarRecordTag.StringVal(nCol);
}

// 添加数据
int NFCRecord::AddRow(const int nRow)
{
    return AddRow(nRow, mVarRecordType);
}

//类型，新值，原值==给原值赋新值，没有就new
bool ValidAdd(VARIANT_TYPE eType, const NFIValueList::VarData& var, std::shared_ptr<NFIValueList::VarData>& pVar)
{
    if (var.nType != eType)
    {
        return false;
    }

    if (!pVar.get())
    {
        if (!NFIValueList::Valid(var))
        {
            return false;
        }

        pVar = std::shared_ptr<NFIValueList::VarData>(new NFIValueList::VarData());
        pVar->nType = eType;
    }

    if (pVar->nType != eType)
    {
        return false;
    }

    pVar->variantData = var.variantData;
   
    return true;
}

int NFCRecord::AddRow(const int nRow, const NFIValueList& var)
{
    int nFindRow = nRow;
    if (nFindRow >= mnMaxRow)
    {
        return -1;
    }

    if (var.GetCount() != GetCols())
    {
        return -1;
    }

    if (nFindRow < 0)
    {
        for (int i = 0; i < mnMaxRow; i++)
        {
            int nUse = mVecUsedState[i];
            if (nUse <= 0)
            {
                nFindRow = i;
                break;
            }
        }
    }

    if (nFindRow < 0)
    {
        return -1;
    }

    bool bType = true;
    for (int i = 0; i < GetCols(); ++i)
    {
        if (var.Type(i) != GetColType(i))
        {
            bType = false;
            break;
        }
    }

    if (bType)
    {
        mVecUsedState[nFindRow] = 1;
        for (int i = 0; i < GetCols(); ++i)
        {
            std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nFindRow, i));//GetVarData(nFindRow, i);
            if(!ValidAdd(GetColType(i), *var.GetStackConst(i), pVar))
            {
                //添加失败--不存在这样的情况，因为类型上面已经监测过，如果返回的话，那么添加的数据是0的话就会返回，导致结果错误
//                 mVecUsedState[nFindRow] = 0;
//                 pVar.reset();
//                 return -1;
            }
        }

        OnEventHandler(mSelf, mstrRecordName, Add, nFindRow, 0, mVarRecordType, var);
    }

    return nFindRow;
}

bool NFCRecord::SetInt(const int nRow, const int nCol, const int value)
{
    if (!ValidPos(nRow, nCol))
    {
        return false;
    }

    NFIValueList::VarData var;
    var.nType = VTYPE_INT;
    var.variantData = value;

    std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nRow, nCol));
    if(!ValidAdd(GetColType(nCol), var, pVar))
    {
        return false;
    }

    if (VTYPE_INT == pVar->nType)
    {
        NFCValueList oldValue;
        NFCValueList newValue;

        oldValue.Append(*pVar);
        newValue.AddInt(value);

        pVar->variantData = value;

        OnEventHandler(mSelf, mstrRecordName, UpData, nRow, nCol, oldValue, newValue);

        return true;
    }

    return false;
}

bool NFCRecord::SetFloat(const int nRow, const int nCol, const float value)
{
    if (!ValidPos(nRow, nCol))
    {
        return false;
    }

    NFIValueList::VarData var;
    var.nType = VTYPE_FLOAT;
    var.variantData = value;

    std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nRow, nCol));
    if(!ValidAdd(GetColType(nCol), var, pVar))
    {
        return false;
    }

    if (VTYPE_FLOAT == pVar->nType)
    {
        NFCValueList oldValue;
        NFCValueList newValue;

        oldValue.Append(*pVar);
        newValue.AddFloat(value);

        pVar->variantData = value;

        OnEventHandler(mSelf, mstrRecordName, UpData, nRow, nCol, oldValue, newValue);

        return true;
    }

    return false;
}

bool NFCRecord::SetDouble(const int nRow, const int nCol, const double value)
{
    if (!ValidPos(nRow, nCol))
    {
        return false;
    }

    NFIValueList::VarData var;
    var.nType = VTYPE_DOUBLE;
    var.variantData = value;

    std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nRow, nCol));
    if(!ValidAdd(GetColType(nCol), var, pVar))
    {
        return false;
    }

    if (VTYPE_DOUBLE == pVar->nType)
    {
        NFCValueList oldValue;
        NFCValueList newValue;

        oldValue.Append(*pVar);
        newValue.AddDouble(value);

        pVar->variantData = value;

        OnEventHandler(mSelf, mstrRecordName, UpData, nRow, nCol, oldValue, newValue);

        return true;
    }

    return false;
}

bool NFCRecord::SetString(const int nRow, const int nCol, const char* value)
{
    if (!ValidPos(nRow, nCol))
    {
        return false;
    }

    NFIValueList::VarData var;
    var.nType = VTYPE_STRING;
    var.variantData = std::string(value);

    std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nRow, nCol));
    if(!ValidAdd(GetColType(nCol), var, pVar))
    {
        return false;
    }

    if (VTYPE_STRING == pVar->nType)
    {
        NFCValueList oldValue;
        NFCValueList newValue;

        oldValue.Append(*pVar);
        newValue.AddString(value);

        pVar->variantData = (std::string)value;

        OnEventHandler(mSelf, mstrRecordName, UpData, nRow, nCol, oldValue, newValue);

        return true;
    }

    return false;
}

bool NFCRecord::SetObject(const int nRow, const int nCol, const NFIDENTID& value)
{
    if (!ValidPos(nRow, nCol))
    {
        return false;
    }

    NFIValueList::VarData var;
    var.nType = VTYPE_OBJECT;
    var.variantData = (NFINT64)value.nData64;

    std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nRow, nCol));
    if(!ValidAdd(GetColType(nCol), var, pVar))
    {
        return false;
    }

    if (VTYPE_OBJECT == pVar->nType)
    {
        NFCValueList oldValue;
        NFCValueList newValue;

        oldValue.Append(*pVar);
        newValue.AddObject(value);

        pVar->variantData = value.nData64;

        OnEventHandler(mSelf, mstrRecordName, UpData, nRow, nCol, oldValue, newValue);

        return true;
    }

    return false;
}

bool NFCRecord::SetPointer(const int nRow, const int nCol, const void* value)
{

    return false;
}

// 获得数据
bool NFCRecord::QueryRow(const int nRow, NFIValueList& varList)
{
    if (!ValidRow(nRow))
    {
        return false;
    }

    varList.Clear();
    for (int i = 0; i < GetCols(); ++i)
    {
        std::shared_ptr<NFIValueList::VarData> pVar = mtRecordVec.at(GetPos(nRow, i));
        if (pVar.get())
        {
            varList.Append(*pVar);
        }
        else
        {
            switch (GetColType(i))
            {
            case VTYPE_INT:
                varList.AddInt(0);
            	break;

            case VTYPE_FLOAT:
                varList.AddFloat(0.0f);
                break;

            case VTYPE_DOUBLE:
                varList.AddDouble(0.0f);
                break;

            case VTYPE_STRING:
                varList.AddString(NULL_STR.c_str());
                break;

            case VTYPE_OBJECT:
                varList.AddObject(0);
                break;
            default:
                return false;
                break;
            }
        }
    }

    if (varList.GetCount() != GetCols())
    {
        return false;
    }

    return true;
}

int NFCRecord::QueryInt(const int nRow, const int nCol) const
{
    if (!ValidPos(nRow, nCol))
    {
        return 0;
    }

    const std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nRow, nCol));
    if (!pVar.get())
    {
        return 0;
    }

    if (VTYPE_INT == pVar->nType)
    {
        return boost::get<int>(pVar->variantData);
    }

    return 0;
}

float NFCRecord::QueryFloat(const int nRow, const int nCol) const
{
    if (!ValidPos(nRow, nCol))
    {
        return 0.0f;
    }

    const std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nRow, nCol));
    if (!pVar.get())
    {
        return 0.0f;
    }

    if (VTYPE_FLOAT == pVar->nType)
    {
        return boost::get<float>(pVar->variantData);
    }

    return 0.0f;
}

double NFCRecord::QueryDouble(const int nRow, const int nCol) const
{
    if (!ValidPos(nRow, nCol))
    {
        return 0.0f;
    }

    const std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nRow, nCol));
    if (!pVar.get())
    {
        return 0.0f;
    }

    if (VTYPE_DOUBLE == pVar->nType)
    {
        return boost::get<double>(pVar->variantData);
    }

    return 0.0f;
}

const std::string& NFCRecord::QueryString(const int nRow, const int nCol) const
{
    if (!ValidPos(nRow, nCol))
    {
        return NULL_STR;
    }

    const std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nRow, nCol));
    if (!pVar.get())
    {
        return NULL_STR;
    }

    if (VTYPE_STRING == pVar->nType)
    {
        return boost::get<const std::string&>(pVar->variantData);
    }

    return NULL_STR;
}

NFIDENTID NFCRecord::QueryObject(const int nRow, const int nCol) const
{
    if (!ValidPos(nRow, nCol))
    {
        return NFIDENTID();
    }

   const  std::shared_ptr<NFIValueList::VarData>& pVar = mtRecordVec.at(GetPos(nRow, nCol));
    if (!pVar.get())
    {
        return NFIDENTID();
    }

    if (VTYPE_OBJECT == pVar->nType)
    {
        return boost::get<NFINT64>(pVar->variantData);
    }

    return NFIDENTID();
}

void* NFCRecord::QueryPointer(const int nRow, const int nCol) const
{
    //     NFIValueList::VarData* pVar = GetVarData(nRow, nCol);
    //     if (pVar && VTYPE_POINTER == pVar->nType)
    //     {
    //         return boost::get<void*>(pVar->variantData);
    //     }

    return NULL;
}

int NFCRecord::FindRowByColValue(const int nCol, const NFIValueList& var, NFIValueList& varResult)
{
    if (!ValidCol(nCol))
    {
        return -1;
    }

    VARIANT_TYPE eType = var.Type(0);
    if (eType != mVarRecordType.Type(nCol))
    {
        return -1;
    }

    switch (eType)
    {
    case VTYPE_INT:
        return FindInt(nCol, var.IntVal(nCol), varResult);
        break;

    case VTYPE_FLOAT:
        return FindFloat(nCol, var.FloatVal(nCol), varResult);
        break;

    case VTYPE_DOUBLE:
        return FindDouble(nCol, var.DoubleVal(nCol), varResult);
        break;

    case VTYPE_STRING:
        return FindString(nCol, var.StringVal(nCol).c_str(), varResult);
        break;

    case VTYPE_OBJECT:
        return FindObject(nCol, var.ObjectVal(nCol), varResult);
        break;

    default:
        break;
    }

    return -1;
}

int NFCRecord::FindInt(const int nCol, const int value, NFIValueList& varResult)
{
    if (VTYPE_INT != mVarRecordType.Type(nCol))
    {
        return -1;
    }

    for (int i = 0; i < mnMaxRow; ++i)
    {
        if (!IsUsed(i))
        {
            continue;
        }

        if (QueryInt(i, nCol) == value)
        {
            varResult << NFINT32(i);
        }
    }

    return varResult.GetCount();
}

int NFCRecord::FindFloat(const int nCol, const float value, NFIValueList& varResult)
{
    if (VTYPE_FLOAT != mVarRecordType.Type(nCol))
    {
        return -1;
    }

    for (int i = 0; i < mnMaxRow; ++i)
    {
        if (!IsUsed(i))
        {
            continue;
        }

        if (QueryFloat(i, nCol) == value)
        {
            varResult << NFINT32(i);
        }
    }

    return varResult.GetCount();
}

int NFCRecord::FindDouble(const int nCol, const double value, NFIValueList& varResult)
{
    if (VTYPE_DOUBLE != mVarRecordType.Type(nCol))
    {
        return -1;
    }

    for (int i = 0; i < mnMaxRow; ++i)
    {
        if (!IsUsed(i))
        {
            continue;
        }

        if (QueryDouble(i, nCol) == value)
        {
            varResult << NFINT32(i);
        }
    }

    return varResult.GetCount();
}

int NFCRecord::FindString(const int nCol, const char* value, NFIValueList& varResult)
{
    if (VTYPE_STRING != mVarRecordType.Type(nCol))
    {
        return -1;
    }

    for (int i = 0; i < mnMaxRow; ++i)
    {
        if (!IsUsed(i))
        {
            continue;
        }

        const std::string& strData = QueryString(i, nCol);
        if (0 == strcmp(strData.c_str(), value))
        {
            varResult << NFINT32(i);
        }
    }

    return varResult.GetCount();
}

int NFCRecord::FindObject(const int nCol, const NFIDENTID& value, NFIValueList& varResult)
{
    if (VTYPE_OBJECT != mVarRecordType.Type(nCol))
    {
        return -1;
    }

    for (int i = 0; i < mnMaxRow; ++i)
    {
        if (!IsUsed(i))
        {
            continue;
        }

        if (QueryObject(i, nCol) == value.nData64)
        {
            varResult << NFINT32(i);
        }
    }

    return varResult.GetCount();
}

int NFCRecord::FindPointer(const int nCol, const void* value, NFIValueList& varResult)
{
    return varResult.GetCount();
}

bool NFCRecord::Remove(const int nRow)
{
    if (ValidRow(nRow))
    {
        if (IsUsed(nRow))
        {
            OnEventHandler(mSelf, mstrRecordName.c_str(), Del, nRow, 0, NFCValueList(), NFCValueList());

            mVecUsedState[nRow] = 0;

//             for (int i = 0; i < GetCols(); ++i)
//             {
//                 std::shared_ptr<NFIValueList::VarData>& var = mtRecordVec.at(GetPos(nRow, i));
//                 var.reset();
//            }

            return true;
        }
    }

    return false;
}

bool NFCRecord::Clear()
{
    for (int i = GetRows() - 1; i >= 0; i--)
    {
        Remove(i);
    }

    return true;
}

void NFCRecord::AddRecordHook(const RECORD_EVENT_FUNCTOR_PTR& cb)
{
    mtRecordCallback.push_back(cb);
}

const bool NFCRecord::GetSave()
{
    return mbSave;
}

const bool NFCRecord::GetPublic()
{
    return mbPublic;
}

const bool NFCRecord::GetPrivate()
{
    return mbPrivate;
}

const int NFCRecord::GetIndex()
{
    return mnIndex;
}

int NFCRecord::GetPos( int nRow, int nCol ) const
{
    return nRow * mVarRecordType.GetCount() + nCol;
}

const std::string& NFCRecord::GetName()
{
    return mstrRecordName;
}

void NFCRecord::SetSave(const bool bSave)
{
    mbSave = bSave;
}

void NFCRecord::SetPublic(const bool bPublic)
{
    mbPublic = bPublic;
}

void NFCRecord::SetPrivate(const bool bPrivate)
{
    mbPrivate = bPrivate;
}

void NFCRecord::SetName(const char* strName)
{
    mstrRecordName = strName;
}

const NFIValueList& NFCRecord::GetInitData()
{
    return mVarRecordType;
}

void NFCRecord::OnEventHandler(const NFIDENTID& self, const std::string& strRecordName, const int nOpType, const int nRow, const int nCol, const NFIValueList& oldVar, const NFIValueList& newVar)
{
    TRECORDCALLBACKEX::iterator itr = mtRecordCallback.begin();
    TRECORDCALLBACKEX::iterator end = mtRecordCallback.end();
    for (; itr != end; ++itr)
    {
        //NFIValueList参数:所属对象名string，操作类型int，Row,Col, OLD属性值，NEW属性值
        RECORD_EVENT_FUNCTOR_PTR functorPtr = *itr;
        functorPtr.get()->operator()(self, strRecordName, nOpType, nRow, nCol, oldVar, newVar, NFCValueList());
    }
}

bool NFCRecord::IsUsed(const int nRow) const
{
    if (ValidRow(nRow))
    {
        return (mVecUsedState[nRow] > 0);
    }

    return false;
}

bool NFCRecord::SwapRowInfo(const int nOriginRow, const int nTargetRow)
{
    if (ValidRow(nOriginRow)
        && ValidRow(nTargetRow))
    {
        for (int i = 0; i < GetCols(); ++i)
        {
            std::shared_ptr<NFIValueList::VarData> pOrigin = mtRecordVec.at(GetPos(nOriginRow, i));
            mtRecordVec[GetPos(nOriginRow, i)] = mtRecordVec.at(GetPos(nTargetRow, i));
            mtRecordVec[GetPos(nTargetRow, i)] = pOrigin;
        }

        int nOriginUse = mVecUsedState[nOriginRow];
        mVecUsedState[nOriginRow] = mVecUsedState[nTargetRow];
        mVecUsedState[nTargetRow] = nOriginUse;

        OnEventHandler(mSelf, mstrRecordName, Changed, nOriginRow, nTargetRow, NFCValueList(), NFCValueList());

        return true;
    }

    return false;
}

const NFIValueList& NFCRecord::GetInitDesc()
{
    return mVarRecordDesc;
}

const NFIValueList& NFCRecord::GetTag()
{
    return mVarRecordTag;
}

bool NFCRecord::SetUsed(const int nRow, const int bUse)
{
    if (ValidRow(nRow))
    {
        mVecUsedState[nRow] = bUse;
        return true;
    }

    return false;
}

bool NFCRecord::SetKey( const int nRow, const int bKey )
{


    return false;
}

bool NFCRecord::IsKey( const int nRow ) const
{

    if (ValidRow(nRow))
    {
        return mVarRecordKey.IntVal(nRow);
        //return (m_pRecordKeyState[nRow] > 0);
    }

    return false;
}

const NFIValueList& NFCRecord::GetKeyState()
{
    return mVarRecordKey;
}

bool NFCRecord::ValidPos( int nRow, int nCol ) const
{
    if (ValidCol(nCol)
        && ValidRow(nRow))
    {
        return true;
    }

    return false;
}

bool NFCRecord::ValidRow( int nRow ) const
{
    if (nRow >= GetRows() || nRow < 0)
    {
        return false;
    }

    return true;
}

bool NFCRecord::ValidCol( int nCol ) const
{
    if (nCol >= GetCols() || nCol < 0)
    {
        return false;
    }

    return true;
}


