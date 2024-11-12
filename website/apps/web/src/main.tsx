import * as Sentry from '@sentry/react'
import { BrowserTracing } from '@sentry/tracing'

import React from 'react'
import ReactDOM from 'react-dom/client'
import ReactGA from 'react-ga-neo'
import { SWRConfig } from 'swr'

import App from './App'
import { fetch } from './utils/fetch'

import './index.css'

Sentry.init({
  dsn: 'https://6e0c079fe21c424189cc0f89ba7bb029@o1299554.ingest.sentry.io/6536125',
  integrations: [new BrowserTracing()],
  tracesSampleRate: 0.01,
})

ReactGA.initialize('G-FJQDKG394Z')

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <SWRConfig
      value={{
        fetcher: (url: string) => fetch(url).then((r) => r.json()),
        focusThrottleInterval: 1000 * 60 * 10,
        suspense: true,
      }}
    >
      <App />
    </SWRConfig>
  </React.StrictMode>,
)
