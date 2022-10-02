package db

import (
	"MaaAssistantArknights/internal/store"
	"sync"
)

type cache struct {
	data map[string]interface{}
	mu   sync.RWMutex
	size int
}

func New() store.Store {
	return &cache{
		data: make(map[string]interface{}),
	}
}

func (c *cache) Set(key string, val interface{}) {
	c.mu.Lock()
	defer c.mu.Unlock()
	if !c.isExists(key) {
		c.size++
	}
	c.data[key] = val
}
func (c *cache) isExists(key string) bool {
	_, ok := c.data[key]
	return ok
}

func (c *cache) Del(key string) {
	c.mu.Lock()
	defer c.mu.Unlock()
	if !c.isExists(key) {
		return
	}
	delete(c.data, key)
	c.size--
}

func (c *cache) Get(key string) (interface{}, bool) {
	c.mu.RLock()
	defer c.mu.RUnlock()
	v, ok := c.data[key]
	return v, ok
}

func (c *cache) Empty() bool {
	c.mu.RLock()
	defer c.mu.RUnlock()
	return c.size == 0
}

func (c *cache) Values() []interface{} {
	c.mu.RLock()
	defer c.mu.RUnlock()
	res := make([]interface{}, 0)

	for _, v := range c.data {
		res = append(res, v)
	}
	return res
}
func (c *cache) Clear() {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.data = nil
	c.size = 0
}
