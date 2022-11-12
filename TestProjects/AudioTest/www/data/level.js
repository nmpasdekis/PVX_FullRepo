(async function (gl){
	const RedColor = new Float32Array([1, 0, 0, 1]);
	const GreenColor = new Float32Array([0, 1, 0, 1]);
	const BlueColor = new Float32Array([0, 0, 1, 1]);
	const WhiteColor = new Float32Array([1, 1, 1, 0.1]);
	const WhiteColorHighlight = new Float32Array([1, 1, 1, 0.3]);
	let fov = 45.0;
	let Projection = gl.perspective(45.0);
	let Camera = gl.math.identity();
	let Rotation = new Float32Array([0, 0, 0]);
	let Ray = new Float32Array([0, 0, 0]);
	let ref = {};

	let PolyTemplate =(()=>{
		let p = gl.CreateProgram2([
			{ type: "VERTEX_SHADER", code: `uniform mat4 view; uniform mat4 projection; attribute vec3 Position; void main(){ gl_Position = projection * view * vec4(Position, 1); }` },
			{ type: "FRAGMENT_SHADER", code: `precision mediump float; uniform vec4 Color; void main(){ gl_FragColor = Color; }` }
		]);
		return {
			primitive: gl.TRIANGLES,
			program: p,
			...gl.buildAttributes(p, [{ "name": "Position", "type": "FLOAT", "size": 3 }])
		}
	})();

	let [Background, Point] = await all([
		gl.FetchMesh("/api/data/background.js", { view: Camera, projection: Projection }),
		gl.FetchMesh("/api/data/point.js", { view: Camera, projection: Projection })
	]);
	let Polygons = [];
	await LoadLevel();

	let Render2 = ()=>{};

	function Render() {
		Background.render();
		Polygons.forEach(poly => {
			if (poly == this.currentPoly) {
				if (poly.points.length > 2) poly.poly.render();
				if(["MovingPoint","EditPoints"].includes(this.state)) poly.points.forEach((p, i) => {
					Point.render({ Position: p, Color: i == poly.points.cur ? RedColor : BlueColor });
				})
			}else{
				if (poly.points.length > 2) poly.poly.render({ Color: ref.highlight==poly ? WhiteColorHighlight: WhiteColor });
			}
		})
	}

	gl.onrelativemove = ((x, y, buttons) => {
		if (buttons & 2) {
			Rotation[1] -= x * 4 * fov / 45; Rotation[0] -= y * 4 * fov / 45;
			Camera.Set(gl.math.rotate_YXZ(Rotation));
			Render2();
		};
	});
	gl.onmousemove = e => {
		Ray.Set(gl.math.castRay(e.relOffsetX, e.relOffsetY, Projection, Camera));
		let h = ref.highlight;
		ref.highlight = Polygons.find(p => gl.math.rayPointDistance2(p.center, Ray) <= p.maxDist2 &&
				p.indices.find(c => {
					let points = c.map(d => p.points[d]);
					let p2 = gl.math.rayPlaneIntersection(gl.math.createPlane(points), Ray);
					return gl.math.pointTriangleIntersection(p2, points)
				}))
		if (h != ref.highlight) Render2();
	}
	Render();

	function BuildPoly(p){
		if (p.points.length > 2) {
			p.indices = gl.math.getPolygonIndices(p.points);
			p.poly.Vertices = p.points;
			p.poly.Indices = p.indices;
			p.center = gl.math.mul(p.points.reduce((a,b) => gl.math.add(a,b), [0,0,0]),1.0/p.points.length);
			p.maxDist2 = p.points.reduce((a, b) => Math.max(a, gl.math.length2(gl.math.sub(b, p.center))),0);
		}
	}

	function raySelectPoly(){
		this.currentPoly = ref.highlight;
		//this.currentPoly = this.Polygons.find(p => gl.math.rayPointDistance2(p.center, Ray)<=p.maxDist2 && 
		//	p.indices.find(c => gl.math.pointTriangleIntersection(Ray, c.map(d => p.points[d]))));
		this.Render();
	}

	function selectPolygon(i){
		console.log(this);
		this.currentPoly = this.Polygons[i];
		this.SetState(null);
		Render2();
	}
	function addPoly() {
		let p = { name: "Poly", points: [], poly: gl.mesh(PolyTemplate), color: new Float32Array([0,1,0,0.3]) }; p.points.cur = -1;
		p.poly.Bindings = {
			view: Camera,
			projection: Projection,
			Color: p.color
		}
		this.Polygons.push(p);
		this.currentPoly = p;
		this.Render();
		return p;
	}
	function removePoly(i) {
		if(this.currentPoly==this.Polygons[i]) this.currentPoly = null;
		this.Polygons.splice(i, 1);
		this.Render();
	}
	function selectPoint(p){
		let i = (p.points.map((c, i) => ({ i: i, d: gl.math.rayPointDistance2(c, Ray) })).filter(c => c.d < 0.00005).orderBy(c => c.d)[0] || { i: -1 }).i;
		if(i != -1){
			p.points.cur = i;
		} else {
			p.points.cur++;
			p.points.splice(p.points.cur,0, Array.from(Ray));
			BuildPoly(p);
		}
		this.Render();
	}
	function movePoint(p){
		p.points[p.points.cur] = Array.from(Ray);
		BuildPoly(p);
		this.Render();
	}
	function zoom(z){
		fov*=z;
		if(fov>45.0) fov=45.0;
		if(fov<5.0) fov=5.0;
		Projection.Set(gl.perspective(fov));
		this.Render();
	}
	function SaveLevel(){
		fetch("/api/data/save/FirstLevel.json",{
			method: "POST",
			headers: {"Content-Type": "application/json"},
			body: JSON.stringify(Polygons.map(c => ({ 
				name: c.name, 
				points:c.points, 
				color: Array.from(c.color) 
			})))
		});
	}

	async function LoadLevel() {
		let dt = (await fetch("/api/data/FirstLevel.json").then(c => c.ok && c.json())) || [];
		Polygons.forEach(c => c.poly.release());
		Polygons.length=0;
		dt.forEach(c => {
			c.color = new Float32Array(c.color); 
			c.poly = gl.mesh(PolyTemplate); 
			c.poly.Bindings = {
				view: Camera,
				projection: Projection,
				Color: c.color
			}
			BuildPoly(c);
			Polygons.push(c)
		});
	}

	function setState(s){
		this.state = s;
		this.Render();
	}

	let ret = {
		Polygons: Polygons,
		currentPoly: null,
		state: null,
		ref: ref,
		get ray() { return Ray; },
		get Render() { return Render; },
		get AddPolygon() { return addPoly; },
		get SelectPoint() { return selectPoint; },
		get SelectPolygon() { return selectPolygon; },
		get MovePoint() { return movePoint; },
		get Zoom() { return zoom; },
		get SaveLevel() { return SaveLevel; },
		get RemovePolygon() { return removePoly; },
		get debug(){ return ()=>{ debugger; } },
		get SetState() { return setState; },
		get RaySelectPoly() { return raySelectPoly; }
	}
	Render2= ret.Render.bind(ret);
	return ret;
})