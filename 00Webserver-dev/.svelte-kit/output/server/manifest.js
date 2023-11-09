export const manifest = {
	appDir: "_app",
	appPath: "_app",
	assets: new Set(["cellularNoise.jpg","dashboard.jpg","favicon.png","robots.txt","simplexNoise.jpg","simplexRNoise.jpg","text.jpg"]),
	mimeTypes: {".jpg":"image/jpeg",".png":"image/png",".txt":"text/plain"},
	_: {
		client: {"start":"_app/immutable/entry/start.15bd57aa.js","app":"_app/immutable/entry/app.8443480d.js","imports":["_app/immutable/entry/start.15bd57aa.js","_app/immutable/chunks/index.b8c32cc7.js","_app/immutable/chunks/singletons.9bbca374.js","_app/immutable/chunks/index.497fad04.js","_app/immutable/entry/app.8443480d.js","_app/immutable/chunks/index.b8c32cc7.js"],"stylesheets":[],"fonts":[]},
		nodes: [
			() => import('./nodes/0.js'),
			() => import('./nodes/1.js')
		],
		routes: [
			
		],
		matchers: async () => {
			
			return {  };
		}
	}
};
