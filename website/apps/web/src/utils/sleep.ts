const sleep = (timeout: number) =>
  new Promise((res) => setTimeout(res, timeout))
export default sleep
