export type WithChildren<T> = T & { children?: React.ReactNode }
export type FCC<T = {}> = React.FC<WithChildren<T>>
