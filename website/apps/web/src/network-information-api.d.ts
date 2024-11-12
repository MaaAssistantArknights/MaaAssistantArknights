declare interface Navigator extends NavigatorNetworkInformation {}
declare interface WorkerNavigator extends NavigatorNetworkInformation {}
declare interface NavigatorNetworkInformation {
  readonly connection?: NetworkInformation
}
type Megabit = number
type Millisecond = number
type EffectiveConnectionType = '2g' | '3g' | '4g' | 'slow-2g'
type ConnectionType =
  | 'bluetooth'
  | 'cellular'
  | 3
  | 4
  | 'ethernet'
  | 1
  | 'mixed'
  | 'none'
  | 'other'
  | 'unknown'
  | 0
  | 'wifi'
  | 2
  | 'wimax'
interface NetworkInformation extends EventTarget {
  readonly type?: ConnectionType
  readonly effectiveType?: EffectiveConnectionType
  readonly downlinkMax?: Megabit
  readonly downlink?: Megabit
  readonly rtt?: Millisecond
  readonly saveData?: boolean
  onchange?: EventListener
}
