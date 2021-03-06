/******************************************************************************
WHAIS - An advanced database system
Copyright(C) 2014-2018  Iulian Popa

Address: Str Olimp nr. 6
         Pantelimon Ilfov,
         Romania
Phone:   +40721939650
e-mail:  popaiulian@gmail.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#include <cassert>
#include <cstring>
#include <vector>
#include <algorithm>

#include "utils/wfile.h"
#include "utils/endianness.h"
#include "utils/wutf.h"
#include "dbs/dbs_mgr.h"
#include "dbs_exception.h"
#include "ps_table.h"
#include "ps_serializer.h"


using namespace std;

namespace whais {
namespace pastra {


static const char PS_TEMP_TABLE_SUFFIX[]   = "pttable_";
static const char PS_TABLE_FIXFIELDS_EXT[] = "_f";
static const char PS_TABLE_VARFIELDS_EXT[] = "_v";
static const uint8_t PS_TABLE_SIGNATURE[]  = { 0x50, 0x41, 0x53, 0x54, 0x52, 0x41, 0x54, 0x42 };

static const uint_t PS_HEADER_SIZE = 128;

static const uint_t PS_TABLE_SIG_OFF               = 0;
static const uint_t PS_TABLES_SIG_LEN              = 8;
static const uint_t PS_TABLE_FIELDS_COUNT_OFF      = 8;
static const uint_t PS_TABLE_FIELDS_COUNT_LEN      = 4;
static const uint_t PS_TABLE_ELEMS_SIZE_OFF        = 12;
static const uint_t PS_TABLE_ELEMS_SIZE_LEN        = 4;
static const uint_t PS_TABLE_ROWS_COUNT_OFF        = 16;
static const uint_t PS_TABLE_ROWS_COUNT_LEN        = 8;
static const uint_t PS_TABLE_MAX_FILE_SIZE_OFF     = 24;
static const uint_t PS_TABLE_MAX_FILE_SIZE_LEN     = 8;
static const uint_t PS_TABLE_MAINTABLE_SIZE_OFF    = 32;
static const uint_t PS_TABLE_MAINTABLE_SIZE_LEN    = 8;
static const uint_t PS_TABLE_VARSTORAGE_SIZE_OFF   = 40;
static const uint_t PS_TABLE_VARSTORAGE_SIZE_LEN   = 8;
static const uint_t PS_TABLE_BT_ROOT_OFF           = 48;
static const uint_t PS_TABLE_BT_ROOT_LEN           = 4;
static const uint_t PS_TABLE_BT_HEAD_OFF           = 52;
static const uint_t PS_TABLE_BT_HEAD_LEN           = 4;
static const uint_t PS_TABLE_ROW_SIZE_OFF          = 56;
static const uint_t PS_TABLE_ROW_SIZE_LEN          = 4;
static const uint_t PS_TABLE_FLAGS_OFF             = 60;
static const uint_t PS_TABLE_FLAGS_LEN             = 4;

static const uint_t PS_RESEVED_FOR_FUTURE_OFF   = 64;
static const uint_t PS_RESEVED_FOR_FUTURE_LEN   = PS_HEADER_SIZE - PS_RESEVED_FOR_FUTURE_OFF;

static const uint32_t PS_TABLE_MODIFIED_MASK    = 1;
static const uint32_t PS_TABLE_TO_REPAIR_MASK   = 2;




static const char*
field_type_to_text(const uint_t type)
{
  if (IS_ARRAY(type))
  {
    switch (GET_BASE_TYPE(type))
    {
    case T_BOOL:
      return "BOOL ARRAY";

    case T_CHAR:
      return "CHAR ARRAY";

    case T_DATE:
      return "DATE ARRAY";

    case T_DATETIME:
      return "DATETIME ARRAY";

    case T_HIRESTIME:
      return "HIRESTIME ARRAY";

    case T_UINT8:
      return "UINT8 ARRAY";

    case T_UINT16:
      return "UINT16 ARRAY";

    case T_UINT32:
      return "UINT32 ARRAY";

    case T_UINT64:
      return "UINT64 ARRAY";

    case T_INT8:
      return "INT8 ARRAY";

    case T_INT16:
      return "INT16 ARRAY";

    case T_INT32:
      return "INT32 ARRAY";

    case T_INT64:
      return "INT64 ARRAY";

    case T_REAL:
      return "REAL ARRAY";

    case T_RICHREAL:
      return "RICHREAL ARRAY";

    default:
      assert(false);
    }
  }
  else
  {
    switch (GET_BASE_TYPE(type))
    {
    case T_BOOL:
      return "BOOL";

    case T_CHAR:
      return "CHAR";

    case T_DATE:
      return "DATE";

    case T_DATETIME:
      return "DATETIME";

    case T_HIRESTIME:
      return "HIRESTIME";

    case T_UINT8:
      return "UINT8";

    case T_UINT16:
      return "UINT16";

    case T_UINT32:
      return "UINT32";

    case T_UINT64:
      return "UINT64";

    case T_INT8:
      return "INT8";

    case T_INT16:
      return "INT16";

    case T_INT32:
      return "INT32";

    case T_INT64:
      return "INT64";

    case T_REAL:
      return "REAL";

    case T_RICHREAL:
      return "RICHREAL";

    case T_TEXT:
      return "TEXT";

    default:
      assert(false);
    }
  }

  throw DBSException(_EXTRA(DBSException::GENERAL_CONTROL_ERROR),
                     "Unexpected field type encountered(%u).",
                     type);
}


static uint_t
get_fields_names_len(const DBSFieldDescriptor* fields, uint_t count)
{
  uint_t result = 0;

  while (count-- > 0)
    {
      result += strlen(fields[0].name) + 1;
      ++fields;
    }

  return result;
}


static bool
validate_field_name(const char* name, bool failException = true)
{

  while (name[0])
  {
    if ( !( ('a' <= name[0] && name[0] <= 'z')
            || ('A' <= name[0] && name[0] <= 'Z')
            || ('0' <= name[0] && name[0] <= '9')
            || name[0] == '_'))
    {
      if (failException )
      {
        throw DBSException(_EXTRA(DBSException::FIELD_NAME_INVALID),
                           "Cannot use '%s' as a field name.",
                           name);
      }
      else
        return false;
    }
    ++name;
  }
  return true;
}


static void
validate_field_descriptors(const DBSFieldDescriptor* const fields, const uint_t fieldsCount)
{
  assert((fieldsCount > 0) && (fields != nullptr));

  for (uint_t i = 0; i < fieldsCount; ++i)
  {
    validate_field_name(fields[i].name);

    // Check that all fields have different names.
    for (uint_t j = i + 1; j < fieldsCount; ++j)
    {
      if (strcmp(fields[i].name, fields[j].name) == 0)
      {
        throw DBSException(_EXTRA(DBSException::FIELD_NAME_INVALID),
                            "Table field name '%s' is duplicated.",
                            fields[i].name);
      }
    }

    if ((fields[i].type == T_UNKNOWN) || (fields[i].type >= T_END_OF_TYPES))
    {
      throw DBSException(_EXTRA(DBSException::FIELD_TYPE_INVALID),
                         "Table field '%s' has invalid type '%X'.",
                         fields[i].name,
                         fields[i].type);
    }


    if (fields[i].isArray && (fields[i].type == T_TEXT))
    {
      throw DBSException(_EXTRA(DBSException::FIELD_TYPE_INVALID),
                         "This implementation does not support array of "
                           "text type for field '%s'.",
                         fields[i].name);
    }
  }
}


static bool
compare_fields(const DBSFieldDescriptor& f1, const DBSFieldDescriptor& f2)
{
  assert(strcmp(f1.name, f2.name) != 0);
  return strcmp(f1.name, f2.name) < 0;
}


static void
normalize_fields(vector<DBSFieldDescriptor>&   fields,
                 uint_t* const                 outRowsSize,
                 uint8_t* const                outFields)
{
  const uint_t fieldsCount = fields.size();

  assert(fieldsCount > 0);

  FieldDescriptor* const fieldsDesc = _RC(FieldDescriptor*, outFields);
  uint_t fieldNameOff = sizeof(fieldsDesc[0]) * fieldsCount;

  memset(outFields, 0, sizeof(fieldsDesc[0]) * fieldsCount);
  sort(fields.begin(), fields.end(), compare_fields);

  *outRowsSize = (fieldsCount + 7) / 8;

  for (uint_t i = 0; i < fieldsCount; i++)
  {
    fieldsDesc[i].NullBitIndex(i);
    fieldsDesc[i].RowDataOff(*outRowsSize);
    fieldsDesc[i].NameOffset(fieldNameOff);
    fieldsDesc[i].Type(fields[i].type | (fields[i].isArray ? PS_TABLE_ARRAY_MASK : 0));

    const uint_t nameLen = strlen(fields[i].name) + 1;

    assert(nameLen > 1);

    memcpy(outFields + fieldNameOff, fields[i].name, nameLen);
    fieldNameOff += nameLen;

    *outRowsSize += Serializer::Size(fields[i].type, fields[i].isArray != FALSE);
  }
}


static void
create_table_file(const uint64_t                    maxFileSize,
                  const char* const                 filePrefix,
                  const DBSFieldDescriptor* const   inoutFields,
                  const uint_t                      fieldsCount)
{
  //Check the arguments
  if ((inoutFields == nullptr) || (fieldsCount == 0) || (fieldsCount > 0xFFFFu))
    {
      throw DBSException(
          _EXTRA(DBSException::OPER_NOT_SUPPORTED),
          "Could not create a persistent table with %d fields count.",
          (inoutFields == nullptr ) ? 0 : fieldsCount
                         );
    }

  //Compute the table header descriptor size
  const uint32_t descriptorsSize = sizeof(FieldDescriptor) * fieldsCount
                                   + get_fields_names_len(inoutFields, fieldsCount);
  validate_field_descriptors(inoutFields, fieldsCount);

  vector<DBSFieldDescriptor> vect(inoutFields + 0, inoutFields + fieldsCount);
  unique_ptr<uint8_t[]> fieldsDescs(unique_array_make(uint8_t, descriptorsSize));
  uint_t rowSize;

  normalize_fields(vect, &rowSize, fieldsDescs.get());

  File tableFile(filePrefix, WH_FILECREATE_NEW | WH_FILERDWR);

  unique_ptr<uint8_t[]> tableHeader(unique_array_make(uint8_t, PS_HEADER_SIZE));
  uint8_t* const header = tableHeader.get();

  memcpy(header, PS_TABLE_SIGNATURE, sizeof PS_TABLE_SIGNATURE);

  store_le_int32(fieldsCount,     header + PS_TABLE_FIELDS_COUNT_OFF);
  store_le_int32(descriptorsSize, header + PS_TABLE_ELEMS_SIZE_OFF);
  store_le_int64(0,               header + PS_TABLE_ROWS_COUNT_OFF);
  store_le_int64(0,               header + PS_TABLE_VARSTORAGE_SIZE_OFF);
  store_le_int32(rowSize,         header + PS_TABLE_ROW_SIZE_OFF);
  store_le_int32(NIL_NODE,        header + PS_TABLE_BT_ROOT_OFF);
  store_le_int32(NIL_NODE,        header + PS_TABLE_BT_HEAD_OFF);
  store_le_int64(maxFileSize,     header + PS_TABLE_MAX_FILE_SIZE_OFF);
  store_le_int64(~(uint64_t)0,    header + PS_TABLE_MAINTABLE_SIZE_OFF);
  store_le_int32(0,               header + PS_TABLE_FLAGS_OFF);

  assert(sizeof(NODE_INDEX) == PS_TABLE_BT_HEAD_LEN);
  assert(sizeof(NODE_INDEX) == PS_TABLE_BT_ROOT_LEN);

  memset(header + PS_RESEVED_FOR_FUTURE_OFF, 0, PS_RESEVED_FOR_FUTURE_LEN);

  //Write the first header part to reserve the space!
  tableFile.Write(header, PS_HEADER_SIZE);

  //Write the field descriptors;
  tableFile.Write(fieldsDescs.get(), descriptorsSize);

  uint64_t minFileSize = (tableFile.Tell() + TableRmNode::RAW_NODE_SIZE - 1);
  minFileSize /= TableRmNode::RAW_NODE_SIZE;
  minFileSize *= TableRmNode::RAW_NODE_SIZE;

  tableFile.Size(minFileSize);

  store_le_int64(tableFile.Size(), header + PS_TABLE_MAINTABLE_SIZE_OFF);

  tableFile.Seek(0, WH_SEEK_BEGIN);
  tableFile.Write(header, PS_HEADER_SIZE);
}


static uint_t
repair_table_fields(FieldDescriptor* const           fields,
                    uint_t                           fieldsCount,
                    const FIX_ERROR_CALLBACK         fixCallback)
{
  static const uint_t NOT_FIXED = 0;
  const auto descriptorBase = _RC(const char*, fields);
  uint_t rowSize = (fieldsCount + 7) / 8;

  for (uint_t i = 0; i < fieldsCount; ++i)
  {
    const char* const fieldName = descriptorBase + fields[i].NameOffset();

    if (fields[i].NullBitIndex() != i)
    {
      const bool fix = fixCallback(FIX_QUESTION,
                                   "Detecting invalid null bit index for field '%s'.",
                                   fieldName);
      if (fix)
        fields[i].NullBitIndex(i);

      else
        return NOT_FIXED;
    }

    if (fields[i].RowDataOff() != rowSize)
    {
      const bool fix = fixCallback(FIX_QUESTION,
                                   "Detected invalid data offset for field '%s'."
                                     " It should be set at %u.",
                                   fieldName,
                                   rowSize);
      if (fix)
        fields[i].RowDataOff(rowSize);

      else
        return NOT_FIXED;
    }
    else
      fixCallback(INFORMATION, "Field '%s' data offset set at '%u'.", fieldName, rowSize);

    rowSize += Serializer::Size(_SC(DBS_FIELD_TYPE, GET_BASE_TYPE(fields[i].Type())),
                                IS_ARRAY(fields[i].Type()));
  }
  return rowSize;
}


static bool
repair_table_header(const string&              name,
                    const string&              fileNamePrefix,
                    const uint64_t             maxFileSize,
                    const FIX_ERROR_CALLBACK   fixCallback)

{
  File tableFile(fileNamePrefix.c_str(), WH_FILEOPEN_EXISTING | WH_FILERDWR);

  const uint64_t tableFileSize = tableFile.Size();
  if (tableFileSize < PS_HEADER_SIZE)
  {
    fixCallback(CRITICAL,
                "The table '%s' cannot be repaired. Its header file is too damaged.",
                name.c_str());
    return false;
  }

  unique_ptr<uint8_t[]> tableHeader(unique_array_make(uint8_t, PS_HEADER_SIZE));
  uint8_t* const header = tableHeader.get();

  tableFile.Seek(0, WH_SEEK_BEGIN);
  tableFile.Read(header, PS_HEADER_SIZE);

  if (memcmp(header, PS_TABLE_SIGNATURE, sizeof PS_TABLE_SIGNATURE) != 0)
  {
    fixCallback(CRITICAL,
                "The table '%s' cannot be repaired. Cannot find the table file's signature.",
                name.c_str());
    return false;
  }

  const uint_t fieldsCount = load_le_int32(header + PS_TABLE_FIELDS_COUNT_OFF);
  const uint_t descSize = load_le_int32(header + PS_TABLE_ELEMS_SIZE_OFF);
  uint_t fileSize = (PS_HEADER_SIZE + descSize + TableRmNode::RAW_NODE_SIZE - 1);

  fileSize /= TableRmNode::RAW_NODE_SIZE;
  fileSize *= TableRmNode::RAW_NODE_SIZE;

  if (fieldsCount == 0
      || fieldsCount > 0xFFFFu
      || (fieldsCount * sizeof(FieldDescriptor) >= descSize))
  {
    fixCallback(CRITICAL,
                "The table '%s' cannot be repaired. The field descriptors are too damaged.",
                name.c_str());
    return false;
  }

  unique_ptr<uint8_t[]> fieldsDescs(unique_array_make(uint8_t, descSize));
  uint8_t* const descriptors = fieldsDescs.get();

  if (tableFile.Size() < PS_HEADER_SIZE + descSize)
  {
    fixCallback(CRITICAL,
                "The table '%s' cannot be repaired. The header file is too damaged.",
                name.c_str());
    return false;
  }

  tableFile.Read(descriptors, descSize);

  uint_t nameOffset = fieldsCount * sizeof(FieldDescriptor);
  FieldDescriptor* const fds = _RC(FieldDescriptor*, descriptors);
  for (uint_t i = 0; i < fieldsCount; ++i)
  {
    if (fds[i].NameOffset() != nameOffset)
    {
      bool fix = fixCallback(FIX_QUESTION,
                             "The table field '%u' is damaged. Its name should be '%s'.",
                             i,
                             descriptors + nameOffset);
      if (fix)
      {
        fds[i].NameOffset(nameOffset);
        if (validate_field_name(_RC(const char*, descriptors + nameOffset)))
        {
          fixCallback(INFORMATION, "Field name set to '%s'.", descriptors + nameOffset);
        }
        else
        {
          fixCallback(CRITICAL, "The restored field name is not valid.");

          return false;
        }
      }
      else
        return false;
    }

    fixCallback(INFORMATION,
                "Found field '%s' of type '%s'.",
                _RC(const char*, descriptors + nameOffset),
                field_type_to_text(fds[i].Type()));

    nameOffset += strlen(_RC(const char*, descriptors + nameOffset)) + 1;
    if (nameOffset > descSize)
    {
       fixCallback(CRITICAL,
                   "The table '%s' cannot be repaired. Its field descriptor is too damaged.",
                   name.c_str());
      return false;
    }
  }

  if (nameOffset != descSize)
  {
    fixCallback(CRITICAL,
                "The table '%s' cannot be repaired. The field descriptors are too damaged.",
                name.c_str());
    return false;
  }

  const uint_t rowSize = repair_table_fields(fds, fieldsCount, fixCallback);
  if (rowSize == 0)
    return false;

  else if (rowSize != load_le_int32(header + PS_TABLE_ROW_SIZE_OFF))
  {
    bool fix = fixCallback(FIX_QUESTION,
                           "The table '%s' row size is set at %u bytes instead of %u.",
                           name.c_str(),
                           load_le_int32(header + PS_TABLE_ROW_SIZE_OFF),
                           rowSize);
    if (fix)
      store_le_int32(rowSize, header + PS_TABLE_ROW_SIZE_OFF);

    else
      return false;
  }
  else
  {
    fixCallback(INFORMATION, "The row size of table '%s' is %u bytes long.", name.c_str(), rowSize);
  }

  //Remove the information about recyclable row.
  //That structure is not reliable.
  store_le_int32(NIL_NODE, header + PS_TABLE_BT_ROOT_OFF);
  store_le_int32(NIL_NODE, header + PS_TABLE_BT_HEAD_OFF);

  uint64_t vsSize = load_le_int64(header + PS_TABLE_VARSTORAGE_SIZE_OFF);

  vsSize /= sizeof(StoreEntry);
  vsSize *= sizeof(StoreEntry);

  store_le_int64(vsSize, header + PS_TABLE_VARSTORAGE_SIZE_OFF);

  tableFile.Seek(0, WH_SEEK_BEGIN);
  tableFile.Write(header, PS_HEADER_SIZE);
  tableFile.Write(descriptors, descSize);

  tableFile.Size(fileSize);
  tableFile.Close();

  FileContainer::Fix(fileNamePrefix.c_str(), maxFileSize, fileSize);
  FileContainer::Fix((fileNamePrefix + PS_TABLE_VARFIELDS_EXT).c_str(), maxFileSize, vsSize);

  return true;
}


class RepairTableNodeManager : public TemporalTable
{
  /* This class is declared to reuse as much as possible the code used to build
   *  the index of a table removed rows. Nothing else! */

public:
  RepairTableNodeManager(DbsHandler& dbs, IDataContainer& container)
    : TemporalTable(dbs, &RepairTableNodeManager::field, 1),
      mContainer(container),
      mCurrentRoot(NIL_NODE)
  {
  }

  ~RepairTableNodeManager()
  {
    FlushInternal();
  }

  virtual uint64_t NodeRawSize() const
  {
    return TableRmNode::RAW_NODE_SIZE;
  }

  virtual NODE_INDEX AllocateNode(const NODE_INDEX parrent, const KEY_INDEX  parrentKey)
  {
    assert(mContainer.Size() % NodeRawSize() == 0);

    return NodeRawSize() / TableRmNode::RAW_NODE_SIZE;
  }


  virtual void FreeNode(const NODE_INDEX nodeId)
  {
    //For the purposes of this class,
    //this member function should not be called
    assert(false);

    throw DBSException(_EXTRA(DBSException::GENERAL_CONTROL_ERROR),
                       "Asked to execute an unexpected function.");
  }

  virtual NODE_INDEX RootNodeId()
  {
    if (mCurrentRoot == NIL_NODE)
    {
      auto rootNode = RetrieveNode(AllocateNode(NIL_NODE, 0));

      rootNode->Next(NIL_NODE);
      rootNode->Prev(NIL_NODE);
      rootNode->KeysCount(0);
      rootNode->Leaf(true);
      rootNode->InsertKey(rootNode->SentinelKey());

      RootNodeId(rootNode->NodeId());
    }

    return mCurrentRoot;
  }

  virtual void RootNodeId(const NODE_INDEX nodeId)
  {
    mCurrentRoot = nodeId;
  }

private:

  virtual uint_t MaxCachedNodes()
  {
    return 0;
  }

  virtual std::shared_ptr<IBTreeNode> LoadNode(const NODE_INDEX nodeId)
  {
    std::shared_ptr<IBTreeNode> node(shared_make(TableRmNode, *this, nodeId));

    if (nodeId < mContainer.Size() / NodeRawSize())
      mContainer.Read(nodeId * NodeRawSize(), NodeRawSize(), node->RawData());

    else
    {
      assert(nodeId == mContainer.Size() / NodeRawSize());

      mContainer.Write(nodeId * NodeRawSize(), NodeRawSize(), node->RawData());
    }

    node->MarkClean();
    assert(node->NodeId() == nodeId);

    return node;
  }

  virtual void SaveNode(IBTreeNode* const node)
  {
    if ( ! node->IsDirty())
      return;

    mContainer.Write(node->NodeId() * NodeRawSize(), NodeRawSize(), node->RawData());

    node->MarkClean();
  }

  IDataContainer&   mContainer;
  NODE_INDEX        mCurrentRoot;

  static DBSFieldDescriptor field;
};

DBSFieldDescriptor RepairTableNodeManager::field = {"dummy", T_BOOL, FALSE};

PersistentTable::PersistentTable(DbsHandler& dbs, const string& name)
  : PrototypeTable(dbs),
    mDbsSettings(DBSGetSeettings()),
    mMaxFileSize(0),
    mVSDataSize(0),
    mFileNamePrefix(dbs.WorkingDir() + name),
    mVSData(nullptr),
    mRemoved(false)
{
  InitFromFile(name);

  if (mMaxFileSize != dbs.MaxFileSize())
  {
    throw DBSException(_EXTRA(DBSException::TABLE_INCONSITENCY),
                       "Persistent table '%s' is set to use a different maximum file size than"
                         " what is parameterized (%lu vs. %lu).",
                       name.c_str(),
                       _SC(long, mMaxFileSize),
                       _SC(long, dbs.MaxFileSize()));
  }

  assert(mTableData.get() != nullptr);

  uint_t blkSize = DBSSettings().mTableCacheBlkSize;
  const uint_t blkCount = DBSSettings().mTableCacheBlkCount;

  assert((blkSize != 0) && (blkCount != 0));

  while (blkSize < mRowSize)
    blkSize *= 2;

  mRowCache.Init(*this, mRowSize, blkSize, blkCount, false);

  InitVariableStorages();
  InitIndexedFields();
}


PersistentTable::PersistentTable(DbsHandler&                       dbs,
                                 const string&                     name,
                                 const DBSFieldDescriptor* const   inoutFields,
                                 const uint_t                      fieldsCount)
  : PrototypeTable(dbs),
    mDbsSettings(DBSGetSeettings()),
    mMaxFileSize(0),
    mVSDataSize(0),
    mFileNamePrefix(dbs.WorkingDir() + name),
    mVSData(nullptr),
    mRemoved(false)
{
  create_table_file(dbs.MaxFileSize(), mFileNamePrefix.c_str(), inoutFields, fieldsCount);
  InitFromFile(name);

  assert(mTableData.get() != nullptr);

  uint_t blkSize = DBSSettings().mTableCacheBlkSize;
  const uint_t blkCount = DBSSettings().mTableCacheBlkCount;

  assert((blkSize != 0) && (blkCount != 0));

  while (blkSize < mRowSize)
    blkSize *= 2;

  mRowCache.Init(*this, mRowSize, blkSize, blkCount, false);

  InitVariableStorages();
  InitIndexedFields();
}


PersistentTable::~PersistentTable()
{
  Flush();

  for (FIELD_INDEX fieldIndex = 0; fieldIndex < mFieldsCount; ++fieldIndex)
  {
    if (mvIndexNodeMgrs[fieldIndex] != nullptr)
    {
      FieldDescriptor& field = GetFieldDescriptorInternal(fieldIndex);

      uint64_t unitsCount = mMaxFileSize - 1;

      unitsCount += mvIndexNodeMgrs[fieldIndex]->IndexRawSize();
      unitsCount /= mMaxFileSize;

      field.IndexUnitsCount(unitsCount);
      delete mvIndexNodeMgrs[fieldIndex];
    }
  }
  MakeHeaderPersistent();
}


bool
PersistentTable::IsTemporal() const
{
  return false;
}


ITable&
PersistentTable::Spawn() const
{
  ITable* const result = new TemporalTable(*this);

  mDbs.RegisterTableSpawn();

  return *result;
}


void
PersistentTable::InitFromFile(const string& tableName)
{
  uint64_t mainTableSize = 0;
  uint8_t tableHdr[PS_HEADER_SIZE];

  File mainTableFile(mFileNamePrefix.c_str(), WH_FILEOPEN_EXISTING | WH_FILEREAD);

  mainTableFile.Seek(0, WH_SEEK_BEGIN);
  mainTableFile.Read(tableHdr, PS_HEADER_SIZE);

  if (memcmp(tableHdr, PS_TABLE_SIGNATURE, PS_TABLES_SIG_LEN) != 0)
  {
    throw DBSException(_EXTRA(DBSException::TABLE_INVALID),
        "Persistent table file '%s' has an invalid signature.", mFileNamePrefix.c_str());
  }

  mFieldsCount     = load_le_int32(tableHdr + PS_TABLE_FIELDS_COUNT_OFF);
  mDescriptorsSize = load_le_int32(tableHdr + PS_TABLE_ELEMS_SIZE_OFF);
  mRowsCount       = load_le_int64(tableHdr + PS_TABLE_ROWS_COUNT_OFF);
  mVSDataSize      = load_le_int64(tableHdr + PS_TABLE_VARSTORAGE_SIZE_OFF);
  mRowSize         = load_le_int32(tableHdr + PS_TABLE_ROW_SIZE_OFF);
  mRootNode        = load_le_int32(tableHdr + PS_TABLE_BT_ROOT_OFF);
  mUnallocatedHead = load_le_int32(tableHdr + PS_TABLE_BT_HEAD_OFF);
  mMaxFileSize     = load_le_int64(tableHdr + PS_TABLE_MAX_FILE_SIZE_OFF);
  mainTableSize    = load_le_int64(tableHdr + PS_TABLE_MAINTABLE_SIZE_OFF);

  if (mFieldsCount == 0
     || mDescriptorsSize < sizeof(FieldDescriptor) * mFieldsCount
     || mainTableSize < PS_HEADER_SIZE)
    {
      throw DBSException(_EXTRA(DBSException::TABLE_INVALID),
                         "Persistent table file '%s' has an invalid signature.",
                         mFileNamePrefix.c_str());
    }
  else if (load_le_int32(tableHdr + PS_TABLE_FLAGS_OFF) & PS_TABLE_MODIFIED_MASK)
    {
      throw DBSException(_EXTRA(DBSException::TABLE_IN_USE),
                         "Cannot open table '%s' as is already in use or was not closed properly"
                          " last time.",
                         tableName.c_str());
    }

  //Cache the field descriptors in memory
  mFieldsDescriptors.reset(new uint8_t[mDescriptorsSize]);
  mainTableFile.Read(_CC(uint8_t*, mFieldsDescriptors.get()), mDescriptorsSize);
  mainTableFile.Close();

  mTableData.reset(new FileContainer(mFileNamePrefix.c_str(),
                                     mMaxFileSize,
                                     (mainTableSize + mMaxFileSize - 1) / mMaxFileSize,
                                     false));
}

void
PersistentTable::InitVariableStorages()
{
  // Loading the rows regular should be done up front.
  mRowsData.reset (new FileContainer((mFileNamePrefix + PS_TABLE_FIXFIELDS_EXT).c_str(),
                                     mMaxFileSize,
                                     ((mRowSize * mRowsCount) + mMaxFileSize - 1) / mMaxFileSize,
                                     false));

  //Check if are fields demanding variable size store.
  for (FIELD_INDEX i = 0; i < mFieldsCount; ++i)
  {
    DBSFieldDescriptor fieldDesc = DescribeField(i);

    assert((fieldDesc.type > T_UNKNOWN) && (fieldDesc.type < T_UNDETERMINED));

    if (fieldDesc.isArray || (fieldDesc.type == T_TEXT))
    {
      mVSData = shared_make(VariableSizeStore);
      mVSData->Init((mFileNamePrefix + PS_TABLE_VARFIELDS_EXT).c_str(),
                    mVSDataSize,
                    mMaxFileSize);

      //We only need one field to require variable storage initialisation
      //and it would be enough for the(if they are present).
      break;
    }
  }
}

void
PersistentTable::InitIndexedFields()
{
  for (FIELD_INDEX fieldIndex = 0; fieldIndex < mFieldsCount; ++fieldIndex)
  {
    FieldDescriptor& field = GetFieldDescriptorInternal(fieldIndex);

    if (field.IndexNodeSizeKB() == 0)
    {
      assert(field.IndexUnitsCount() == 0);

      mvIndexNodeMgrs.push_back(nullptr);
      continue;
    }

    const string containerName = mFileNamePrefix
                                 + '_'
                                 + _RC(const char*, mFieldsDescriptors.get() + field.NameOffset())
                                 + "_bt";

    unique_ptr<IDataContainer> indexContainer(unique_make(FileContainer,
                                                          containerName.c_str(),
                                                          mMaxFileSize,
                                                          field.IndexUnitsCount(),
                                                          false));
    mvIndexNodeMgrs.push_back(new FieldIndexNodeManager(indexContainer,
                                                        field.IndexNodeSizeKB() * 1024,
                                                        0x400000, //4MB
                                                        _SC(DBS_FIELD_TYPE, field.Type()),
                                                        false));
  }
}

void
PersistentTable::MakeHeaderPersistent()
{
  if (mRemoved)
    return ; //We were removed. We were removed.

  uint32_t flags = 0;

  if (mRowModified)
    flags |= PS_TABLE_MODIFIED_MASK;

  uint8_t tableHdr[PS_HEADER_SIZE];

  memcpy(tableHdr, PS_TABLE_SIGNATURE, sizeof PS_TABLE_SIGNATURE);

  store_le_int32(mFieldsCount,        tableHdr + PS_TABLE_FIELDS_COUNT_OFF);
  store_le_int32(mDescriptorsSize,    tableHdr + PS_TABLE_ELEMS_SIZE_OFF);
  store_le_int64(mRowsCount,          tableHdr + PS_TABLE_ROWS_COUNT_OFF);
  store_le_int32(mRowSize,            tableHdr + PS_TABLE_ROW_SIZE_OFF);
  store_le_int32(mRootNode,           tableHdr + PS_TABLE_BT_ROOT_OFF);
  store_le_int32(mUnallocatedHead,    tableHdr + PS_TABLE_BT_HEAD_OFF);
  store_le_int64(mMaxFileSize,        tableHdr + PS_TABLE_MAX_FILE_SIZE_OFF);
  store_le_int64(mTableData->Size(),  tableHdr + PS_TABLE_MAINTABLE_SIZE_OFF);
  store_le_int32(flags,               tableHdr + PS_TABLE_FLAGS_OFF);

  store_le_int64((mVSData != nullptr) ? mVSData->Size() : 0,
                 tableHdr + PS_TABLE_VARSTORAGE_SIZE_OFF);

  memset(tableHdr + PS_RESEVED_FOR_FUTURE_OFF, 0, PS_RESEVED_FOR_FUTURE_LEN);

  mTableData->Write(0, sizeof tableHdr, tableHdr);
  mTableData->Write(sizeof tableHdr, mDescriptorsSize, mFieldsDescriptors.get());
}


void
PersistentTable::RemoveFromDatabase()
{
  if (mRowsData.get() != nullptr)
    mRowsData->MarkForRemoval();

  if (mVSData != nullptr)
    mVSData->MarkForRemoval();

  for (FIELD_INDEX i = 0; i < mFieldsCount; ++i)
  {
    if (mvIndexNodeMgrs[i] != nullptr)
      mvIndexNodeMgrs[i]->MarkForRemoval();
  }

  mTableData->MarkForRemoval();
  mRemoved = true;
}


IDataContainer*
PersistentTable::CreateIndexContainer(const FIELD_INDEX field)
{
  assert(!mFileNamePrefix.empty());

  const DBSFieldDescriptor desc = DescribeField(field);
  const string containerNameBase = mFileNamePrefix + '_' + desc.name + "_bt";

  return new FileContainer(containerNameBase.c_str(), mDbsSettings.mMaxFileSize, 0, false);
}


void
PersistentTable::FlushEpilog()
{
  if (mVSData != nullptr)
    mVSData->Flush();

  if (mRowsData.get() != nullptr)
    mRowsData->Flush();

  if (mTableData.get() != nullptr)
    mTableData->Flush();
}


IDataContainer&
PersistentTable::RowsContainer()
{
  assert(mRowsData.get() != nullptr);

  return *mRowsData.get();
}


IDataContainer&
PersistentTable::TableContainer()
{
  assert(mTableData.get() != nullptr);

  return *mTableData.get();
}


VariableSizeStoreSPtr
PersistentTable::VSStore()
{
  assert(mVSData);

  return mVSData;
}


bool
PersistentTable::ValidateTable(const std::string& path, const std::string& name)
{
  uint8_t tableHdr[PS_HEADER_SIZE];
  bool    fix = false;

  const string tableFileName = path + name;

  File tableFile(tableFileName.c_str(), WH_FILEOPEN_EXISTING | WH_FILERDWR);

  tableFile.Seek(0, WH_SEEK_BEGIN);
  tableFile.Read(tableHdr, PS_HEADER_SIZE);

  const uint32_t fieldsCount = load_le_int32(tableHdr + PS_TABLE_FIELDS_COUNT_OFF);

  if ((memcmp(tableHdr, PS_TABLE_SIGNATURE, PS_TABLES_SIG_LEN) != 0)
      || (load_le_int32(tableHdr + PS_TABLE_FIELDS_COUNT_OFF) == 0)
      || (load_le_int32(tableHdr + PS_TABLE_ELEMS_SIZE_OFF) <
          (sizeof(FieldDescriptor) * fieldsCount))
      || (load_le_int64(tableHdr + PS_TABLE_MAINTABLE_SIZE_OFF) <
          PS_HEADER_SIZE))
    {
      fix = true;
    }

  uint32_t tableFlags = load_le_int32(tableHdr + PS_TABLE_FLAGS_OFF);
  if (tableFlags & PS_TABLE_MODIFIED_MASK)
    fix = true;

  if (fix)
  {
    tableFlags |= PS_TABLE_TO_REPAIR_MASK;
    store_le_int32(tableFlags, tableHdr + PS_TABLE_FLAGS_OFF);
  }

  tableFile.Seek(0, WH_SEEK_BEGIN);
  tableFile.Write(tableHdr, sizeof tableHdr);

  return (fix == false);
}

static bool
check_array_buffer(const uint8_t* const   buffer,
                   const uint_t           bufferSize,
                   const DBS_FIELD_TYPE   itemType)
{
  assert(2 * sizeof(uint64_t) == Serializer::Size(T_RICHREAL, true));

  if ((bufferSize >= 2 * sizeof(uint64_t)) || (bufferSize == 0))
    return false;

  else if (bufferSize != (buffer[2 * sizeof(uint64_t) - 1] & 0x7F))
    return false;

  const uint_t itemSize = Serializer::Size(itemType, false);

  if (bufferSize % itemSize != 0)
    return false;

  const Serializer::VALUE_VALIDATOR validator = Serializer::SelectValidator(itemType);

  uint_t itemOffset = 0;
  while (itemOffset < bufferSize)
  {
    if (!validator(buffer + itemOffset))
      return false;

    itemOffset += itemSize;
  }

  return itemOffset == bufferSize;
}


static bool
check_text_buffer(const uint8_t* const utf8buffer, const uint_t bufferSize)
{
  assert(2 * sizeof(uint64_t) == Serializer::Size(T_TEXT, false));

  if ((bufferSize >= 2 * sizeof(uint64_t)) || (bufferSize == 0))
    return false;

  else if (bufferSize != (utf8buffer[2 * sizeof(uint64_t) - 1] & 0x7F))
    return false;

  uint_t verified = 0;
  while (verified < bufferSize)
  {
    try
    {
      const uint_t unitsCount = wh_utf8_cu_count(utf8buffer[verified]);

      if (unitsCount == 0)
        return false;

      uint32_t cp;
      wh_load_utf8_cp(utf8buffer + verified, &cp);
      DChar chk_cp_valid(cp);

      verified += unitsCount;
    }
    catch (...)
    {
      return false;
    }
  }

  return verified == bufferSize;
}


static void
remove_extra_container_files(FIX_ERROR_CALLBACK   fixCallback,
                             const string         baseFile,
                             const uint64_t       containerSize,
                             const uint64_t       maxFileSize)
{
  uint_t seriesStart = containerSize / maxFileSize + 1;

  if ((containerSize == 0) || (containerSize % maxFileSize == 0))
    --seriesStart;

  while (true)
  {
    string fileName = baseFile;

    if (seriesStart != 0)
      append_int_to_str(seriesStart, fileName);

    if ( ! whf_file_exists(fileName.c_str()))
      return;

    fixCallback(INFORMATION, "Removing extra file '%s.", fileName.c_str());
    if ( ! whf_remove(fileName.c_str()))
    {
      throw WFileContainerException(_EXTRA(WFileContainerException::FILE_OS_IO_ERROR),
                                    "Failed to remove file '%s'.",
                                    fileName.c_str());
    }
    ++seriesStart;
  }
}


bool
PersistentTable::RepairTable(DbsHandler&           dbs,
                             const std::string&    name,
                             const std::string&    path,
                              FIX_ERROR_CALLBACK   fixCallback)
{
  const DBSSettings& settings = dbs.Settings();

  const string fileNamePrefix = path + name;

  if ( ! repair_table_header(name, fileNamePrefix, settings.mMaxFileSize, fixCallback))
    return false;

  File tableFile(fileNamePrefix.c_str(),
  WH_FILEOPEN_EXISTING | WH_FILERDWR);

  assert(tableFile.Size() >= TableRmNode::RAW_NODE_SIZE);

  unique_ptr<uint8_t[]> tableHeader(unique_array_make(uint8_t, PS_HEADER_SIZE));

  tableFile.Seek(0, WH_SEEK_BEGIN);
  tableFile.Read(tableHeader.get(), PS_HEADER_SIZE);

  assert(memcmp(tableHeader.get(), PS_TABLE_SIGNATURE, sizeof PS_TABLE_SIGNATURE) == 0);

  const uint_t fieldsCount = load_le_int32(tableHeader.get() + PS_TABLE_FIELDS_COUNT_OFF);
  const uint_t descSize = load_le_int32(tableHeader.get() + PS_TABLE_ELEMS_SIZE_OFF);
  const uint32_t rowSize = load_le_int32(tableHeader.get() + PS_TABLE_ROW_SIZE_OFF);

  uint32_t rowsCount = load_le_int32(tableHeader.get() + PS_TABLE_ROWS_COUNT_OFF);
  uint64_t vsDataSize = load_le_int64(tableHeader.get() + PS_TABLE_VARSTORAGE_SIZE_OFF);

  unique_ptr<uint8_t[]> fieldsDescs(unique_array_make(uint8_t, descSize));

  assert(tableFile.Size() >= PS_HEADER_SIZE + descSize);

  tableFile.Read(fieldsDescs.get(), descSize);
  tableFile.Close();

  const auto fds = _RC(FieldDescriptor*, fieldsDescs.get());

  std::vector<FieldIndexNodeManager*> indexNodeMgrs;
  for (FIELD_INDEX i = 0; i < fieldsCount; ++i)
  {
    if ((fds[i].IndexNodeSizeKB() == 0)
        || (fds[i].IndexUnitsCount() == 0))
      {
        fds[i].IndexNodeSizeKB(0);
        fds[i].IndexUnitsCount(0);

        indexNodeMgrs.push_back(nullptr);
        continue;
      }

    fds[i].IndexUnitsCount(0);

    const string containerName = fileNamePrefix
                                 + '_'
                                 + (_RC(const char*, fds) + fds[i].NameOffset())
                                 + "_bt";

    FileContainer::Fix(containerName.c_str(), settings.mMaxFileSize, 0);
    unique_ptr<IDataContainer> indexContainer(unique_make(FileContainer,
                                                          containerName.c_str(),
                                                          settings.mMaxFileSize,
                                                          0,
                                                          false));
    indexNodeMgrs.push_back(new FieldIndexNodeManager(indexContainer,
                                                      fds[i].IndexNodeSizeKB() * 1024,
                                                      0x400000, //4MB
                                                      _SC(DBS_FIELD_TYPE, fds[i].Type()),
                                                      true));
  }

  FileContainer tableData(fileNamePrefix.c_str(), settings.mMaxFileSize, 1, false);
  FileContainer rowsData((fileNamePrefix + PS_TABLE_FIXFIELDS_EXT).c_str(),
                         settings.mMaxFileSize,
                         ((rowSize * rowsCount) + settings.mMaxFileSize - 1) / settings.mMaxFileSize,
                         false);

  RepairTableNodeManager tableNodeMgr(dbs, tableData);

  if (rowSize * rowsCount != rowsData.Size())
  {
    const bool fix = fixCallback(FIX_QUESTION,
                                "The table's row data does not match table header descriptions.");
    if (! fix )
      return false;

    rowsCount = min<uint64_t> (rowsData.Size() / rowSize, rowsCount);

    fixCallback(INFORMATION, "Set the table rows count at '%u'.", rowsCount);

    rowsData.Colapse(rowsCount * rowSize, rowsData.Size());
  }
  else
    fixCallback(INFORMATION, "Table '%s' has %u row(s) allocated.", name.c_str(), rowsCount);

  unique_ptr<VariableSizeStore> vsData(unique_make(VariableSizeStore));
  if (vsDataSize > 0)
  {
    vsData->Init((fileNamePrefix + PS_TABLE_VARFIELDS_EXT).c_str(),
                 vsDataSize,
                 settings.mMaxFileSize);
    vsData->PrepareToCheckStorage();
  }

  unique_ptr<uint8_t[]> _d(unique_array_make(uint8_t, rowSize));
  uint8_t* const rowData = _d.get();
  for (ROW_INDEX row = 0; row < rowsCount; ++row)
  {
    NODE_INDEX dummyNode;
    KEY_INDEX dummyKey;

    bool allFieldsAreNull = true;

    rowsData.Read(row * rowSize, rowSize, rowData);

    for (FIELD_INDEX field = 0; field < fieldsCount; ++field)
    {
      const uint_t byteOff = fds[field].NullBitIndex() / 8;
      const uint_t bitOff = fds[field].NullBitIndex() % 8;
      const uint8_t* const fieldData = rowData + fds[field].RowDataOff();

      bool isNullValue = ((rowData[byteOff] & (1 << bitOff)) != 0);

      if (IS_ARRAY(fds[field].Type()) && !isNullValue)
      {
        const uint64_t fieldEntry = load_le_int64(fieldData);
        const uint64_t fieldSize = load_le_int64(fieldData + sizeof(uint64_t));
        if (fieldSize & 0x8000000000000000ull)
        {
          if ( ! check_array_buffer(fieldData,
                                    (fieldSize >> 56) & 0x7F,
                                    _SC(DBS_FIELD_TYPE, GET_BASE_TYPE(fds[field].Type()))))

          {
            fixCallback(FIX_INFO,
                        "Detected invalid value of field '%s' at row %u. Set to nullptr.",
                        fieldsDescs.get() + fds[field].NameOffset(), row);
            isNullValue = true;
          }
        }
        else if ( ! vsData->CheckArrayEntry(fieldEntry,
                                            fieldSize,
                                            _SC(DBS_FIELD_TYPE, GET_BASE_TYPE(fds[field].Type()))))
        {
          fixCallback(FIX_INFO, "Detected invalid value of field '%s' at row"
              " %u. Set to nullptr.", fieldsDescs.get() + fds[field].NameOffset(), row);
          isNullValue = true;
        }
      }
      else if (GET_BASE_TYPE(fds[field].Type()) == T_TEXT && ! isNullValue)
      {
        const uint64_t fieldEntry = load_le_int64(fieldData);
        const uint64_t fieldSize = load_le_int64(fieldData + sizeof(uint64_t));
        if ((fieldSize & 0x8000000000000000ull) != 0)
        {
          if ( ! check_text_buffer(fieldData, (fieldSize >> 56) & 0x7F))
          {
            fixCallback(FIX_INFO,
                        "Detected invalid value of field '%s' at row %u. Set to nullptr.",
                        fieldsDescs.get() + fds[field].NameOffset(), row);
            isNullValue = true;
          }
        }
        else if (!vsData->CheckTextEntry(fieldEntry, fieldSize))
        {
          fixCallback(FIX_INFO,
                      "Detected invalid value of field '%s' at row %u. Set to nullptr.",
                      fieldsDescs.get() + fds[field].NameOffset(), row);
          isNullValue = true;
        }
      }
      else if ((indexNodeMgrs[field] != nullptr))
      {
        switch (GET_BASE_TYPE(fds[field].Type()))
        {
        case T_BOOL:
        {
          DBool value;

          if (!isNullValue && !Serializer::ValidateDBoolBuffer(fieldData))
          {
            fixCallback(FIX_INFO,
                        "Detected invalid value of field '%s' at row %u. Set to nullptr.",
                        fieldsDescs.get() + fds[field].NameOffset(), row);
            isNullValue = true;
          }

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DBool>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_CHAR:
        {
          DChar value;

          if ( ! isNullValue && ! Serializer::ValidateDCharBuffer(fieldData))
          {
            fixCallback(FIX_INFO,
                        "Detected invalid value of field '%s' at row %u. Set to nullptr.",
                        fieldsDescs.get() + fds[field].NameOffset(), row);
            isNullValue = true;
          }

          if (!isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DChar>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_DATE:
        {
          DDate value;

          if ( ! isNullValue && ! Serializer::ValidateDDateBuffer(fieldData))
          {
            fixCallback(FIX_INFO,
                        "Detected invalid value of field '%s' at row %u. Set to nullptr.",
                        fieldsDescs.get() + fds[field].NameOffset(), row);
            isNullValue = true;
          }

          if (!isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DDate>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_DATETIME:
        {
          DDateTime value;

          if ( ! isNullValue && ! Serializer::ValidateDDateTimeBuffer(fieldData))
          {
            fixCallback(FIX_INFO,
                        "Detected invalid value of field '%s' at row %u. Set to nullptr.",
                        fieldsDescs.get() + fds[field].NameOffset(), row);
            isNullValue = true;
          }

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DDateTime>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_HIRESTIME:
        {
          DHiresTime value;

          if ( ! isNullValue && ! Serializer::ValidateDHiresTimeBuffer(fieldData))
          {
            fixCallback(FIX_INFO,
                        "Detected invalid value of field '%s' at row %u. Set to nullptr.",
                        fieldsDescs.get() + fds[field].NameOffset(), row);
            isNullValue = true;
          }

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DHiresTime>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_INT8:
        {
          DInt8 value;

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DInt8>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_INT16:
        {
          DInt16 value;

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DInt16>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_INT32:
        {
          DInt32 value;

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DInt32>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_INT64:
        {
          DInt64 value;

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DInt64>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_REAL:
        {
          DReal value;

          if ( ! isNullValue && ! Serializer::ValidateDRealBuffer(fieldData))
          {
            fixCallback(FIX_INFO,
                        "Detected invalid value of field '%s' at row %u. Set to nullptr.",
                        fieldsDescs.get() + fds[field].NameOffset(),
                        row);
            isNullValue = true;
          }

          if (!isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DReal>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_RICHREAL:
        {
          DRichReal value;

          if ( ! isNullValue && ! Serializer::ValidateDRichRealBuffer(fieldData))
          {
            fixCallback(FIX_INFO,
                        "Detected invalid value of field '%s' at row %u. Set to nullptr.",
                        fieldsDescs.get() + fds[field].NameOffset(),
                        row);
            isNullValue = true;
          }

          if (!isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DRichReal>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_UINT8:
        {
          DUInt8 value;

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DUInt8>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_UINT16:
        {
          DUInt16 value;

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DUInt16>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_UINT32:
        {
          DUInt32 value;

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DUInt32>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        case T_UINT64:
        {
          DUInt64 value;

          if ( ! isNullValue)
            Serializer::Load(fieldData, &value);

          BTree(*indexNodeMgrs[field]).InsertKey(T_BTreeKey<DUInt64>(value, row),
                                                 &dummyNode,
                                                 &dummyKey);
        }
          break;

        default:
          throw DBSException(_EXTRA(DBSException::GENERAL_CONTROL_ERROR));
        }
      }

      if (isNullValue)
        rowData[byteOff] |= (1 << bitOff);

      allFieldsAreNull &= isNullValue;
    }
    rowsData.Write(row * rowSize, rowSize, rowData);

    if (allFieldsAreNull)
    {
      BTree removedNodes(tableNodeMgr);
      TableRmKey key(row);

      removedNodes.InsertKey(key, &dummyNode, &dummyKey);
    }
  }

  if (vsDataSize)
  {
    vsData->ConcludeStorageCheck();
    vsDataSize = vsData->Size();
  }

  remove_extra_container_files(fixCallback,
                               (fileNamePrefix + PS_TABLE_VARFIELDS_EXT),
                               vsData->Size(),
                               settings.mMaxFileSize);
  remove_extra_container_files(fixCallback,
                               (fileNamePrefix + PS_TABLE_FIXFIELDS_EXT),
                               rowsData.Size(),
                               settings.mMaxFileSize);

  store_le_int64(rowsCount, tableHeader.get() + PS_TABLE_ROWS_COUNT_OFF);
  store_le_int64(vsDataSize, tableHeader.get() + PS_TABLE_VARSTORAGE_SIZE_OFF);
  store_le_int32(tableNodeMgr.RootNodeId(), tableHeader.get() + PS_TABLE_BT_ROOT_OFF);
  store_le_int64(tableData.Size(), tableHeader.get() + PS_TABLE_MAINTABLE_SIZE_OFF);

  store_le_int32(0, tableHeader.get() + PS_TABLE_FLAGS_OFF);

  for (FIELD_INDEX field = 0; field < fieldsCount; ++field)
  {
    if (indexNodeMgrs[field] == nullptr)
    {
      assert(fds[field].IndexNodeSizeKB() == 0);
      assert(fds[field].IndexUnitsCount() == 0);

      continue;
    }

    assert(indexNodeMgrs[field]->IndexRawSize() > 0);
    assert(fds[field].IndexNodeSizeKB() > 0);
    assert(fds[field].IndexUnitsCount() == 0);

    uint64_t unitsCount = indexNodeMgrs[field]->IndexRawSize();
    unitsCount += settings.mMaxFileSize - 1;
    unitsCount /= settings.mMaxFileSize;

    fds[field].IndexUnitsCount(unitsCount);
    delete indexNodeMgrs[field];
  }

  tableData.Write(0, PS_HEADER_SIZE, tableHeader.get());
  tableData.Write(PS_HEADER_SIZE, descSize, fieldsDescs.get());

  return true;
}


TemporalTable::TemporalTable(DbsHandler&                       dbs,
                             const DBSFieldDescriptor* const   inoutFields,
                             const FIELD_INDEX                 fieldsCount)
  : PrototypeTable(dbs),
    mVSData(nullptr)
{

  //Check the arguments
  if (inoutFields == nullptr
      || fieldsCount == 0
      || fieldsCount > 0xFFFFu)
    {
      throw DBSException(
          _EXTRA(DBSException::OPER_NOT_SUPPORTED),
          "Could not create a temporal table with %d fields count.",
          (inoutFields == nullptr ) ? 0 : fieldsCount
                         );
    }

  //Compute the table header descriptor size
  const uint32_t descriptorsSize = sizeof(FieldDescriptor) * fieldsCount +
                                   get_fields_names_len(inoutFields, fieldsCount);

  validate_field_descriptors(inoutFields, fieldsCount);

  vector<DBSFieldDescriptor> vect(inoutFields + 0, inoutFields + fieldsCount);
  unique_ptr<uint8_t[]> fieldDescs(unique_array_make(uint8_t, descriptorsSize));

  uint_t rowSize;

  normalize_fields(vect, &rowSize, fieldDescs.get());

  mFieldsCount = fieldsCount;
  mDescriptorsSize = descriptorsSize;
  mRowSize = rowSize;
  mFieldsDescriptors.reset(fieldDescs.release());

  mvIndexNodeMgrs.insert(mvIndexNodeMgrs.begin(), mFieldsCount, nullptr);

  uint_t blkSize = DBSSettings().mTableCacheBlkSize;
  const uint_t blkCount = DBSSettings().mTableCacheBlkCount;

  assert((blkSize != 0) && (blkCount != 0));

  while (blkSize < mRowSize)
    blkSize *= 2;

  mRowCache.Init(*this, mRowSize, blkSize, blkCount, true);
}


TemporalTable::TemporalTable(const PrototypeTable& prototype)
  : PrototypeTable(prototype),
    mVSData(nullptr)
{

  mvIndexNodeMgrs.insert(mvIndexNodeMgrs.begin(), mFieldsCount, nullptr);

  uint_t       blkSize  = DBSSettings().mTableCacheBlkSize;
  const uint_t blkCount = DBSSettings().mTableCacheBlkCount;

  assert((blkSize != 0) && (blkCount != 0));

  while (blkSize < mRowSize)
    blkSize *= 2;

  mRowCache.Init(*this, mRowSize, blkSize, blkCount, true);
}

TemporalTable::~TemporalTable()
{
  for (FIELD_INDEX fieldIndex = 0; fieldIndex < mFieldsCount; ++fieldIndex)
    delete mvIndexNodeMgrs[fieldIndex];
}

bool
TemporalTable::IsTemporal() const
{
  return true;
}

ITable&
TemporalTable::Spawn() const
{
  ITable* const result = new TemporalTable((PrototypeTable&)*this);

  mDbs.RegisterTableSpawn();

  return *result;
}

void
TemporalTable::FlushEpilog()
{
  if (mVSData != nullptr)
    mVSData->Flush();
}

void
TemporalTable::MakeHeaderPersistent()
{
  //Do nothing!
}

IDataContainer*
TemporalTable::CreateIndexContainer(const FIELD_INDEX)
{
  return new TemporalContainer();
}

IDataContainer&
TemporalTable::TableContainer()
{
  if (mTableData.get() == nullptr)
    mTableData.reset(new TemporalContainer);

  return *mTableData.get();
}

IDataContainer&
TemporalTable::RowsContainer()
{
  if (mRowsData.get() == nullptr)
    mRowsData.reset(new TemporalContainer);

  return *mRowsData.get();
}

VariableSizeStoreSPtr
TemporalTable::VSStore()
{
  if ( ! mVSData)
  {
    mVSData = shared_make(VariableSizeStore);
    mVSData->Init(mDbs.WorkingDir().c_str(), 4096);
  }

  return mVSData;
}


} //namespace pastra
} //namespace whais

