import { Dispatch } from 'react'
import unfetch from 'unfetch'

export const fetch = window.fetch || unfetch

export type RequestInitWithTimeout = RequestInit & {
  timeout?: number
}

export async function fetchWithTimeout(
  input: RequestInfo,
  init?: RequestInitWithTimeout,
) {
  const { timeout, ...options } = init || {}

  const controller = new AbortController()
  const id = setTimeout(() => controller.abort(), timeout)
  const response = await fetch(input, {
    ...options,
    signal: controller.signal,
  }).finally(() => clearTimeout(id))
  return response
}

// yes i know, download uses XHR, but only XHR provides onprogress callbacks...
export type DownloadOptions = {
  onProgress?: Dispatch<ProgressEvent>

  // ttfb timeout in ms; ttfb is the time between the request being sent and the first byte being received
  ttfbTimeout?: number
}
export async function download(
  url: string,
  options: DownloadOptions = {},
): Promise<Blob> {
  const { onProgress, ttfbTimeout } = options

  const xhr = new XMLHttpRequest()
  xhr.open('GET', url, true)
  xhr.responseType = 'blob'
  xhr.withCredentials = false

  let ttfbTimeoutTimer = ttfbTimeout
    ? setTimeout(() => {
        xhr.abort()
      }, ttfbTimeout)
    : null

  return new Promise((resolve, reject) => {
    xhr.onload = () => {
      if (xhr.status === 200) {
        resolve(xhr.response)
      } else {
        reject(xhr.statusText)
      }
    }

    xhr.onerror = () => {
      reject(xhr.statusText)
    }

    xhr.ontimeout = () => {
      reject('timeout')
    }

    xhr.onabort = () => {
      reject('abort')
    }

    xhr.onloadstart = () => {
      if (ttfbTimeoutTimer) {
        clearTimeout(ttfbTimeoutTimer)
        ttfbTimeoutTimer = null
      }
    }

    if (onProgress) {
      xhr.onprogress = onProgress
    }

    xhr.send()
  })
}

function randomSeed() {
  try {
    return crypto.randomUUID()
  } catch {
    return Math.random().toString(36).substring(2)
  }
}

export async function checkUrlConnectivity(
  url: string,
  timeout = 5000,
): Promise<DOMHighResTimeStamp | false> {
  try {
    const measureName = `checkUrlConnectivity-${randomSeed()}`
    const controller = new AbortController()
    const signal = controller.signal
    setTimeout(() => controller.abort(), Math.max(timeout, 5000))
    performance.mark(`${measureName}-start`)
    const response = await fetch(url, {
      method: 'HEAD',
      signal,
      redirect: 'follow',
    })
    performance.mark(`${measureName}-end`)
    if (!response.ok) {
      return false
    }
    performance.measure(
      measureName,
      `${measureName}-start`,
      `${measureName}-end`,
    )
    var measures = performance.getEntriesByName(measureName)
    var measure = measures[0]
    performance.clearMarks(`${measureName}-start`)
    performance.clearMarks(`${measureName}-end`)
    performance.clearMeasures(measureName)
    return measure.duration
  } catch {
    return false
  }
}

// unit: B/ms
export async function checkUrlSpeed(
  url: string,
  timeout = 5000,
): Promise<number> {
  try {
    const measureName = `checkUrlSpeed-${randomSeed()}`
    const controller = new AbortController()
    const signal = controller.signal
    setTimeout(() => controller.abort(), Math.max(timeout, 5000))
    const response = await fetch(url, {
      method: 'GET',
      signal,
      redirect: 'follow',
      headers: {
        Range: 'bytes=0-524287',
      },
    })
    if (!response.ok) {
      return -1
    }
    if (!response.body) {
      return -1
    }
    const reader = response.body.getReader()
    let totalBytesRead = 0
    performance.mark(`${measureName}-start`)
    while (true) {
      const { done, value } = await reader.read()
      if (done) {
        performance.mark(`${measureName}-end`)
        break
      }
      totalBytesRead += value.length
    }
    performance.measure(
      measureName,
      `${measureName}-start`,
      `${measureName}-end`,
    )
    var measures = performance.getEntriesByName(measureName)
    var measure = measures[0]
    performance.clearMarks(`${measureName}-start`)
    performance.clearMarks(`${measureName}-end`)
    performance.clearMeasures(measureName)
    return totalBytesRead / measure.duration
  } catch {
    return -1
  }
}
