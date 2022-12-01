package store

type Store interface {
	Set(key string, val interface{})
	Get(key string) (interface{}, bool)
	Del(key string)
	Empty() bool
	Values() []interface{}
	Clear()
}
