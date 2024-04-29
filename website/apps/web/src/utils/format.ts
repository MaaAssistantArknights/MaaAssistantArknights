import moment from 'moment'

export const pad = (n: number, width?: number, z?: string) => {
  z = z || '0'
  width = width || 2
  const nstr = n.toString()
  return nstr.padStart(width, z)
}

export const formatDate = (date: string) =>
  moment(date).format('YYYY-MM-DD HH:mm')

export const formatBytes = (bytes: number, decimals = 2) => {
  if (bytes === 0) return '0B'

  const k = 1024
  const dm = decimals < 0 ? 0 : decimals
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB']

  const i = Math.floor(Math.log(bytes) / Math.log(k))

  return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + sizes[i]
}
