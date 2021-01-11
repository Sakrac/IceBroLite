#pragma once

#include <inttypes.h>
#include <assert.h>

template< class KeyType, class ValueType > struct HashTable {

	size_t size;
	size_t maxSteps;
	size_t used;

	KeyType* keys;
	ValueType* values;

	static size_t HashValue(KeyType kv) { uint64_t v = kv; return size_t(((v + (v >> 28) + (v << 28)) + 14695981039346656037ull) * 1099511628211ull); }
	static size_t Slot(size_t hash, size_t tableSize) { return hash & (tableSize - 1); }
	static size_t NextSlot(size_t hash, size_t tableSize) { return (hash + 1) & (tableSize - 1); }
	static size_t HashSlot(KeyType key, size_t tableSize) { return Slot(HashValue(key), tableSize); }
	static size_t FindSlot(KeyType hash, size_t hashTableSize, KeyType* hashKeys) {
		size_t slot = HashSlot(hash, hashTableSize);
		while (KeyType k = hashKeys[slot]) {
			if (k == hash) { return slot; }
			slot = NextSlot(slot, hashTableSize);
		}
		return slot;
	}
	static size_t FindSlot(KeyType hash, size_t hashTableSize, KeyType* hashKeys, size_t maxKeySteps) {
		size_t slot = HashSlot(hash, hashTableSize);
		while (hashKeys) {
			KeyType k = hashKeys[slot];
			if (!k || k == hash) { return slot; }
			slot = NextSlot(slot, hashTableSize);
			if (!maxKeySteps--) { break; }
		}
		return slot;
	}

	size_t HashSlot(KeyType key) { return HashSlot(key, size); }

	size_t InsertSlot(KeyType key, size_t slot) {
		const KeyType* hashKeys = keys;
		size_t currSize = size;
		size_t insertSteps = 0;
		while (KeyType k = hashKeys[slot]) {
			if (k == key) { return slot; }  // key already exists
			size_t kfirst = HashSlot(k, currSize);
			size_t ksteps = kfirst > slot ? (currSize + slot - kfirst) : (slot - kfirst);
			if (insertSteps > ksteps) { return slot; }
			slot = NextSlot(slot, size);
			++insertSteps;
		}
		return slot;
	}

	size_t FindSlot(KeyType hash) const { return FindSlot(hash, size, keys, maxSteps); }

	size_t Steps(KeyType hash) {
		size_t slot = HashSlot(hash, size);
		size_t numSteps = 0;
		while (keys[slot] && keys[slot] != hash) {
			++numSteps;
			slot = NextSlot(slot, size);
		}
		return numSteps;
	}

	void UpdateSteps(size_t first, size_t slot) {
		size_t steps = slot > first ? (slot - first) : (size + slot - first);
		if (steps > maxSteps) { maxSteps = steps; }
	}

	ValueType* InsertFitted(KeyType key) {
		assert(key); // key may not be 0
		size_t first = HashSlot(key);
		size_t slot = InsertSlot(key, first);
		UpdateSteps(first, slot);
		if (keys[slot]) {
			if (keys[slot] == key) { return &values[slot]; } else {
				KeyType prvKey = keys[slot];
				ValueType prev_value = values[slot];
				keys[slot] = key;
				for (;; ) {
					size_t prev_first = HashSlot(prvKey);
					size_t slotRH = InsertSlot(prvKey, prev_first);
					UpdateSteps(prev_first, slotRH);
					if (keys[slotRH] && keys[slotRH] != prvKey) {
						KeyType tmpKey = keys[slotRH];
						keys[slotRH] = prvKey;
						prvKey = tmpKey;
						ValueType temp_value = values[slotRH];
						values[slotRH] = prev_value;
						prev_value = temp_value;
					} else {
						keys[slotRH] = prvKey;
						values[slotRH] = prev_value;
						++used;
						return &values[slot];
					}
				}
			}
		}
		keys[slot] = key;
		++used;
		return &values[slot];
	}

	HashTable() { Reset(); }

	void Reset() {
		used = 0;
		size = 0;
		maxSteps = 0;
		keys = nullptr;
		values = nullptr;
	}

	~HashTable() { Clear(); }

	void Clear() {
		if (values) {
			for (size_t i = 0, n = size; i < n; ++i) {
				values[i].~ValueType();
			}
			free(values);
		}
		if (keys) { free(keys); }
		Reset();
	}

	size_t GetUsed() const { return used; }
	bool TableMax() const { return used && (used << 4) >= (size * 13); }

	void Grow() {
		KeyType* prevKeys = keys;
		ValueType* prevValues = values;
		size_t prevSize = size;
		size_t newSize = prevSize ? (prevSize << 1) : 64;

		size = newSize;
		keys = (KeyType*)calloc(1, newSize * sizeof(KeyType));
		values = (ValueType*)calloc(1, newSize * sizeof(ValueType));
		maxSteps = 0;

//		for (size_t i = 0; i < newSize; ++i) {
//			new (values + i) ValueType;
//		}

		if (used) {
			used = 0;
			for (size_t i = 0; i < prevSize; i++) {
				if (KeyType key = prevKeys[i]) {
					*InsertFitted(key) = prevValues[i];
				}
			}
		}

		if (prevKeys) { free(prevKeys); }
		if (prevValues) {
			for (size_t i = 0; i != prevSize; ++i) {
				prevValues[i].~ValueType();
			}
			free(prevValues);
		}
	}

	ValueType* Insert(KeyType key) {
		if (!size || TableMax()) { Grow(); }
		return InsertFitted(key);
	}

	ValueType* Insert(KeyType key, const ValueType& value) {
		ValueType* value_ptr = Insert(key);
		*value_ptr = value;
		return value_ptr;
	}

	bool Exists(KeyType key) {
		return size && key && keys[FindSlot(key)] == key;
	}

	ValueType* Value(KeyType key) {
		if (size && key) {
			size_t slot = FindSlot(key);
			if (keys[slot] == key) {
				return &values[slot];
			}
		}
		return nullptr;
	}
};

