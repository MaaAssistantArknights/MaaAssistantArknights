import { HomeHero } from './components/home/HomeHero'
import { ThemeProvider } from './contexts/ThemeContext'
import { ThemeToggle } from './components/ThemeToggle'

function App() {
  return (
    <ThemeProvider>
      <main className="h-full max-h-screen overflow-hidden dark:bg-[#080808] bg-[#f5f5f5] transition-colors duration-300">
        <ThemeToggle />
        <section className="h-screen min-h-[20rem] w-full relative">
          <HomeHero />
        </section>
      </main>
    </ThemeProvider>
  )
}

export default App
