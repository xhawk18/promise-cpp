

var a = new Promise((resolve, reject) => {
    reject(15);
    resolve(13);
}).then((x) => {
    console.log('resolved x = ', x);
}, (x) => {
    console.log('rejected x = ', x);
    return 18;
});

var b = a;
var c = b;

/*.then(function(n){
    console.log('n = ', n);
    return 4;
});/**/


new Promise((resolve, reject) => {
    resolve();
}).then(function(){
    console.log(a);
    return c;
}).then(function(x){
    console.log('resolved x = ', x);
    return a;
}, function(x){
    console.log('rejected x = ', x);
}).then(function(x){
    console.log('pre finally x = ', x);
}).then(function(x){
    console.log('finally');
});

