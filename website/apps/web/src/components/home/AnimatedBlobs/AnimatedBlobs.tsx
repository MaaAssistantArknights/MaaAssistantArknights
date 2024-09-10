import clsx from 'clsx'

const blobs = [
  ['bg-red-900 translate-y-36', 'left-[30%]'],
  ['bg-orange-900 translate-y-1', 'left-[45%]'],
  ['bg-blue-900 -translate-y-40', 'left-[55%]'],
  ['bg-cyan-900 translate-y-20', 'left-[68%]'],
]

export function AnimatedBlobs() {
  return (
    <div className="absolute h-full w-full overflow-hidden transform-gpu blur-[9rem] left-[-10rem] top-[-10rem]">
      {blobs.map((blob, i) => (
        <div key={i} className={clsx('absolute top-[50vh]', blob[1])}>
          <div
            className={clsx('h-[20rem] w-[20rem] rounded-full', blob[0])}
          ></div>
        </div>
      ))}
    </div>
  )
}
