const getLength = _ => _.reduce((count, elem) => count + (Array.isArray(elem) ? getLength(elem) : 1), 0);

if (process.argv.length < 3) return console.error("Missing argument `array`");

const arr = JSON.parse(process.argv[2]);
if (!Array.isArray(arr)) return console.error("Argument is not an array");

console.log(`Total length is: ${getLength(arr)}`);
