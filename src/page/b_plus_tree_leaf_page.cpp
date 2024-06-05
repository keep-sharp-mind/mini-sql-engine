#include "page/b_plus_tree_leaf_page.h"

#include <algorithm>

#include "index/generic_key.h"

#define pairs_off (data_)
#define pair_size (GetKeySize() + sizeof(RowId))
#define key_off 0
#define val_off GetKeySize()
/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/

/**
 * TODO: Student Implement
 */
/**
 * Init method after creating a new leaf page
 * Including set page type, set current size to zero, set page id/parent id, set
 * next page id and set max size
 * 未初始化next_page_id
 */
void LeafPage::Init(page_id_t page_id, page_id_t parent_id, int key_size, int max_size) {
	this->SetPageId(page_id);
	this->SetParentPageId(parent_id);
	this->SetMaxSize(max_size);
	this->SetSize(0);
	this->SetKeySize(key_size);
	this->next_page_id_ = INVALID_PAGE_ID;
	this->SetPageType(IndexPageType::LEAF_PAGE);
}

/**
 * Helper methods to set/get next page id
 */
page_id_t LeafPage::GetNextPageId() const {
  return next_page_id_;
}

void LeafPage::SetNextPageId(page_id_t next_page_id) {
  next_page_id_ = next_page_id;
  if (next_page_id == 0) {
    cout << "Fatal error";
  }
}

/**
 * TODO: Student Implement
 */
/**
 * Helper method to find the first index i so that pairs_[i].first >= key
 * NOTE: This method is only used when generating index iterator
 * 二分查找
 */
int LeafPage::KeyIndex(const GenericKey *key, const KeyManager &KM) {
  int i;
  for(i = 0; i < this->GetSize(); i++){
  	if(KM.CompareKeys(key, data_[i].first) <= 0){
  		return i;	
	}
  }
  return i;
}

/*
 * Helper method to find and return the key associated with input "index"(a.k.a
 * array offset)
 */
GenericKey *LeafPage::KeyAt(int index) {
  return data_[index].first;
}

void LeafPage::SetKeyAt(int index, GenericKey *key) {
  data_[index].first = key;
}

RowId LeafPage::ValueAt(int index) const {
  return data_[index].second;
}

void LeafPage::SetValueAt(int index, RowId value) {
  data_[index].second = value;
}

void *LeafPage::PairPtrAt(int index) {
  return KeyAt(index);
}

void LeafPage::PairCopy(void *dest, void *src, int pair_num) {
  memcpy(dest, src, pair_num * (GetKeySize() + sizeof(RowId)));
}
/*
 * Helper method to find and return the key & value pair associated with input
 * "index"(a.k.a. array offset)
 */
std::pair<GenericKey *, RowId> LeafPage::GetItem(int index) { return {KeyAt(index), ValueAt(index)}; }

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert key & value pair into leaf page ordered by key
 * @return page size after insertion
 */
int LeafPage::Insert(GenericKey *key, const RowId &value, const KeyManager &KM) {
  	int key_index = this->KeyIndex(key, KM);
  	if(key_index == this->GetSize()){
    	data_[key_index].first = key;
    	data_[key_index].second = value;
    	this->IncreaseSize(1);
    	return this->GetSize();
  	}
  	if(KM.CompareKeys(data_[key_index].first, key) == 0){
    	return this->GetSize();
  	}
  	std::move_backward(data_ + key_index, data_ + this->GetSize(), data_ + this->GetSize() + 1);
	data_[key_index].first = key;
  	data_[key_index].second = value;

  	this->IncreaseSize(1);
  	return this->GetSize();
}

/*****************************************************************************
 * SPLIT
 *****************************************************************************/
/*
 * Remove half of key & value pairs from this page to "recipient" page
 */
void LeafPage::MoveHalfTo(LeafPage *recipient) {
	int first_half = this->GetMinSize();
  	int second_half = this->GetMaxSize() - first_half;
  	recipient->CopyNFrom(data_ + first_half, second_half);
  	this->IncreaseSize(-second_half);
}

/*
 * Copy starting from items, and copy {size} number of elements into me.
 */
void LeafPage::CopyNFrom(void *src, int size) {
	std::copy(src, src + size, data_ + this->GetSize());
  	this->IncreaseSize(size);
}

/*****************************************************************************
 * LOOKUP
 *****************************************************************************/
/*
 * For the given key, check to see whether it exists in the leaf page. If it
 * does, then store its corresponding value in input "value" and return true.
 * If the key does not exist, then return false
 */
bool LeafPage::Lookup(const GenericKey *key, RowId &value, const KeyManager &KM) {
  int key_index = this->KeyIndex(key, KM);
  if(key_index == this->GetSize() || KM.CompareKeys(this->KeyAt(key_index), key)){
    return false;
  }
  else{
    value = data_[key_index].second; 
    return true;
  }
  return false;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * First look through leaf page to see whether delete key exist or not. If
 * existed, perform deletion, otherwise return immediately.
 * NOTE: store key&value pair continuously after deletion
 * @return  page size after deletion
 */
int LeafPage::RemoveAndDeleteRecord(const GenericKey *key, const KeyManager &KM) {
	int key_index = this->KeyIndex(key, KM);
	if(!(key_index == this->GetSize() || KM.CompareKeys(key, this->KeyAt(key_index)))){
		std::move(data_ + key_index + 1, data_ + GetSize(), data_ + key_index);
		this->IncreaseSize(-1);
	}
	return this->GetSize();
}

/*****************************************************************************
 * MERGE
 *****************************************************************************/
/*
 * Remove all key & value pairs from this page to "recipient" page. Don't forget
 * to update the next_page id in the sibling page
 */
void LeafPage::MoveAllTo(LeafPage *recipient) {
	recipient->CopyNFrom(data_, this->GetSize());
  	recipient->SetNextPageId(this->GetNextPageId());
  	this->IncreaseSize(-1 * this->GetSize());
  	return ;
}

/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to "recipient" page.
 *
 */
void LeafPage::MoveFirstToEndOf(LeafPage *recipient) {
	std::pair<GenericKey *, RowId> first_item = GetItem(0);
  	std::move(data_ + 1, data_ + GetSize(), data_);
  	IncreaseSize(-1);
	recipient->CopyLastFrom(first_item.first, first_item.second);
}

/*
 * Copy the item into the end of my item list. (Append item to my array)
 */
void LeafPage::CopyLastFrom(GenericKey *key, const RowId value) {
	data_[this->GetSize()] = {key, value};
  	this->IncreaseSize(1);
}

/*
 * Remove the last key & value pair from this page to "recipient" page.
 */
void LeafPage::MoveLastToFrontOf(LeafPage *recipient) {
	std::pair<GenericKey *, RowId> last_item = data_[GetSize() - 1];
  	this->IncreaseSize(-1);
  	recipient->CopyFirstFrom(last_item.first, last_item.second);
}

/*
 * Insert item at the front of my items. Move items accordingly.
 *
 */
void LeafPage::CopyFirstFrom(GenericKey *key, const RowId value) {
	std::move(data_, data_ + GetSize(), data_ + GetSize() + 1);
  	data_[0] = {key, value};
  	IncreaseSize(1);
}
