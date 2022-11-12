Float32Array.prototype.Set = function(v){ for(let i=0;i<this.length;i++) this[i] = v[i]||0; }

function Math3D() {
	this.identity = (() => new Float32Array([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]));
	this.scale = (s => new Float32Array([s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1]));
	this.dot3 = ((v1, v2) => v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]);
	this.cross = ((v1, v2) => new Float32Array([v1[1] * v2[2] - v2[1] * v1[2], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0]]));
	this.length = (v => Math.sqrt(v.reduce((acc, c) => acc + c * c, 0)));
	this.length2 = (v => v.reduce((acc, c) => acc + c * c, 0));
	this.normalize = (v => { let il = 1.0 / this.length(v); return v.map(c => c * il); });
	this.add = ((v1, v2) => v1.map((c, i) => c + v2[i]));
	this.sub = ((v1, v2) => v1.map((c, i) => c - v2[i]));
	this.mul = ((v, f) => v.map(c => c * f));
	this.mul3 = ((v, f) => new Float32Array([v[0] * f, v[1] * f, v[2] * f]));
	this.neg = (v => v.map(c => -c));
	this.mul4x4 = ((m1, m2) => new Float32Array([
		m2[0] * m1[0] + m2[1] * m1[4] + m2[2] * m1[8] + m2[3] * m1[12],
		m2[0] * m1[1] + m2[1] * m1[5] + m2[2] * m1[9] + m2[3] * m1[13],
		m2[0] * m1[2] + m2[1] * m1[6] + m2[2] * m1[10] + m2[3] * m1[14],
		m2[0] * m1[3] + m2[1] * m1[7] + m2[2] * m1[11] + m2[3] * m1[15],
		m2[4] * m1[0] + m2[5] * m1[4] + m2[6] * m1[8] + m2[7] * m1[12],
		m2[4] * m1[1] + m2[5] * m1[5] + m2[6] * m1[9] + m2[7] * m1[13],
		m2[4] * m1[2] + m2[5] * m1[6] + m2[6] * m1[10] + m2[7] * m1[14],
		m2[4] * m1[3] + m2[5] * m1[7] + m2[6] * m1[11] + m2[7] * m1[15],
		m2[8] * m1[0] + m2[9] * m1[4] + m2[10] * m1[8] + m2[11] * m1[12],
		m2[8] * m1[1] + m2[9] * m1[5] + m2[10] * m1[9] + m2[11] * m1[13],
		m2[8] * m1[2] + m2[9] * m1[6] + m2[10] * m1[10] + m2[11] * m1[14],
		m2[8] * m1[3] + m2[9] * m1[7] + m2[10] * m1[11] + m2[11] * m1[15],
		m2[12] * m1[0] + m2[13] * m1[4] + m2[14] * m1[8] + m2[15] * m1[12],
		m2[12] * m1[1] + m2[13] * m1[5] + m2[14] * m1[9] + m2[15] * m1[13],
		m2[12] * m1[2] + m2[13] * m1[6] + m2[14] * m1[10] + m2[15] * m1[14],
		m2[12] * m1[3] + m2[13] * m1[7] + m2[14] * m1[11] + m2[15] * m1[15]
	]));
	this.affineInverse = ((m) => {
		return new Float32Array([
			m[0], m[4], m[8], -(m[0] * m[3] + m[4] * m[7] + m[8] * m[11]),
			m[1], m[5], m[9], -(m[1] * m[3] + m[5] * m[7] + m[9] * m[11]),
			m[2], m[6], m[10], -(m[2] * m[3] + m[6] * m[7] + m[10] * m[11]),
			-(m[0] * m[12] + m[1] * m[13] + m[2] * m[14]),
			-(m[4] * m[12] + m[5] * m[13] + m[6] * m[14]),
			-(m[8] * m[12] + m[9] * m[13] + m[10] * m[14]),
			1.0
		]);
	});
	this.affineInverse2 = ((m) => {
		return new Float32Array([
			m[0], m[4], m[8], 0,
			m[1], m[5], m[9], 0,
			m[2], m[6], m[10], 0,
			0, 0, 0, 1.0
		]);
	});
	this.castRay = ((x, y, persp, view) => {
		x /= persp[0];
		y /= persp[5];
		return this.normalize([
			-(x * view[0] + y * view[1] + view[2]),
			-(x * view[4] + y * view[5] + view[6]),
			-(x * view[8] + y * view[9] + view[10])
		]);
	})
	this.makeView = ((x, y, z, pos) => new Float32Array([x[0], y[0], z[0], 0, x[1], y[1], z[1], 0, x[2], y[2], z[2], 0, this.dot3(x, pos), this.dot3(y, pos), this.dot3(z, pos), 1]));
	this.lookAt = ((pos, target, up) => {
		let z = this.normalize(this.sub(target, pos));
		let x = this.normalize(this.cross(up, z));
		let y = this.cross(z, x);
		return this.makeView(x, y, z, pos);
	});
	this.perspective = ((fov, aspect, near, far) => {
		let ys = 1.0 / Math.tan(fov / 2);
		let xs = ys / aspect;
		let nf = near - far;
		return new Float32Array([xs, 0, 0, 0, 0, ys, 0, 0, 0, 0, far / nf, -1, 0, 0, far * near / nf, 0]);
	});
	this.rotate_XYZ = (r => {
		let cp = Math.cos(r[0]);
		let sp = Math.sin(r[0]);
		let cy = Math.cos(r[1]);
		let sy = Math.sin(r[1]);
		let cr = Math.cos(r[2]);
		let sr = Math.sin(r[2]);

		return new Float32Array([
			cr * cy, cy * sr, - sy, 0,
			cr * sp * sy - cp * sr, cp * cr + sp * sr * sy, cy * sp, 0,
			sp * sr + cp * cr * sy, cp * sr * sy - cr * sp, cp * cy, 0,
			0, 0, 0, 1.0
		]);
	});
	this.rotate_XZY = (r => {
		let cp = Math.cos(r[0]);
		let sp = Math.sin(r[0]);
		let cy = Math.cos(r[1]);
		let sy = Math.sin(r[1]);
		let cr = Math.cos(r[2]);
		let sr = Math.sin(r[2]);

		return new Float32Array([
			cr * cy, sr, -cr * sy, 0,
			sp * sy - cp * cy * sr, cp * cr, cy * sp + cp * sr * sy, 0,
			cp * sy + cy * sp * sr, -cr * sp, cp * cy - sp * sr * sy, 0,
			0, 0, 0, 1.0
		]);
	});
	this.rotate_YXZ = (r => {
		let cp = Math.cos(r[0]);
		let sp = Math.sin(r[0]);
		let cy = Math.cos(r[1]);
		let sy = Math.sin(r[1]);
		let cr = Math.cos(r[2]);
		let sr = Math.sin(r[2]);

		return new Float32Array([
			cr * cy - sp * sr * sy, cy * sr + cr * sp * sy, -cp * sy, 0,
			-cp * sr, cp * cr, sp, 0,
			cr * sy + cy * sp * sr, sr * sy - cr * cy * sp, cp * cy, 0,
			0, 0, 0, 1.0
		]);
	});
	this.rotate_YZX = (r => {
		let cp = Math.cos(r[0]);
		let sp = Math.sin(r[0]);
		let cy = Math.cos(r[1]);
		let sy = Math.sin(r[1]);
		let cr = Math.cos(r[2]);
		let sr = Math.sin(r[2]);

		return new Float32Array([
			cr * cy, sp * sy + cp * cy * sr, cy * sp * sr - cp * sy, 0,
			-sr, cp * cr, cr * sp, 0,
			cr * sy, cp * sr * sy - cy * sp, cp * cy + sp * sr * sy, 0,
			0, 0, 0, 1.0
		]);
	});
	this.rotate_ZXY = (r => {
		let cp = Math.cos(r[0]);
		let sp = Math.sin(r[0]);
		let cy = Math.cos(r[1]);
		let sy = Math.sin(r[1]);
		let cr = Math.cos(r[2]);
		let sr = Math.sin(r[2]);

		return new Float32Array([
			cr * cy + sp * sr * sy, cp * sr, cy * sp * sr - cr * sy, 0,
			cr * sp * sy - cy * sr, cp * cr, sr * sy + cr * cy * sp, 0,
			cp * sy, -sp, cp * cy, 0,
			0, 0, 0, 1.0
		]);
	});
	this.rotate_ZYX = (r => {
		let cp = Math.cos(r[0]);
		let sp = Math.sin(r[0]);
		let cy = Math.cos(r[1]);
		let sy = Math.sin(r[1]);
		let cr = Math.cos(r[2]);
		let sr = Math.sin(r[2]);

		return new Float32Array([
			cr * cy, cp * sr + cr * sp * sy, sp * sr - cp * cr * sy, 0,
			-cy * sr, cp * cr - sp * sr * sy, cr * sp + cp * sr * sy, 0,
			sy, -cy * sp, cp * cy, 0,
			0, 0, 0, 1.0
		]);
	});;
	this.rayPointDistance = ((point, rayDir, rayPos = [0, 0, 0]) => {
		let dif = [rayPos[0] - point[0], rayPos[1] - point[1], rayPos[2] - point[2]];
		let dot = this.dot3(dif, rayDir);
		let v = [dif[0] - dot * rayDir[0], dif[1] - dot * rayDir[1], dif[2] - dot * rayDir[2]];
		return this.length(v);
	});
	this.rayPointDistance2 = ((point, rayDir, rayPos = [0, 0, 0]) => {
		let dif = [rayPos[0] - point[0], rayPos[1] - point[1], rayPos[2] - point[2]];
		let dot = this.dot3(dif, rayDir);
		let v = [dif[0] - dot * rayDir[0], dif[1] - dot * rayDir[1], dif[2] - dot * rayDir[2]];
		return this.length2(v);
	});
	this.triangleWinding = (([a, b, c]) => {
		return this.cross(this.sub(b, a), this.sub(c, a));
	});
	this.createPlane = ((tri) => {
		let vec = this.normalize(this.triangleWinding(tri));
		return new Float32Array([...vec, this.dot3(vec, tri[0])]);
	});
	this.pointPlainProjection = ((plane, point) => {
		return this.add(this.mul3(plane, plane[3] - this.dot3(point, plane)), point);
	});
	this.rayPlaneIntersection = ((plane, rayDir, rayPos = [0, 0, 0]) => {
		let num = plane[3] - this.dot3(rayPos, plane);
		let denom = this.dot3(rayDir, plane);
		return this.add(this.mul(rayDir, (num / denom)), rayPos);
	});
	this.pointTriangleIntersection = ((point, [v1, v2, v3]) => {
		let w1 = this.triangleWinding([v1, v2, point]);
		let w2 = this.triangleWinding([v2, v3, point]);
		let w3 = this.triangleWinding([v3, v1, point]);
		return this.dot3(w1, w2) >= 0 && this.dot3(w2, w3) >= 0;
	})
	this.getPolygonIndices = ((verts) => {
		if (verts.length < 3) return [];
		let allIdxs = verts.map((c, i) => i);
		let ret = [];
		let edges = verts.map((c, i) => [i, i + 1]); edges[edges.length - 1][1] = 0;
		let w = this.triangleWinding(verts);
		let secondChance = false;
		while (edges.length > 2) {
			let found = false;
			for (let i = 0; i < edges.length; i++) {
				let j = (i + 1) % edges.length;
				let e1 = edges[i];
				let e2 = edges[j];
				let idxs = [e1[0], e2[0], e2[1]];
				let tryVerts = idxs.map(c => verts[c]);
				let triWind = this.createPlane(tryVerts);
				let otherIdxs = allIdxs.filter(c => !idxs.includes(c));
				if (this.dot3(w, triWind) > 0 && otherIdxs.findIndex(c => {
					return this.pointTriangleIntersection(this.pointPlainProjection(triWind, verts[c]), tryVerts)
				}) == -1) {
					found = true;
					ret.push(idxs);
					if (i < j) {
						edges.splice(j, 1);
						edges[i] = [e1[0], e2[1]];
					} else {
						edges.splice(i, 1);
						edges[j] = [e1[0], e2[1]];
					}
					break;
				}
			}
			if (!found) {
				if (secondChance) return [];
				secondChance = true;
				ret = [];
				edges = verts.map((c, i) => [i, i + 1]); edges[edges.length - 1][1] = 0;
				w = this.neg(w);
			}
		}
		return ret;
	})
}

function getWebGL2Context(canvas){
	let gl = canvas.getContext("webgl2");
	gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
	gl.enable(gl.BLEND);

	const typeSizes = { "FLOAT": 4, "INT": 4 };

	function attrSize(a) { return a.size * typeSizes[a.type]; }
	{
		gl.math = new Math3D();
		gl.perspective = function(deg, near = 0.1, far = 1000.0){
			return gl.math.perspective(Math.PI * deg / 180.0, gl.drawingBufferWidth / (1.0 * gl.drawingBufferHeight), near, far);
		}
		gl.CreateShader = function ({ code, type }) {
			let ret = gl.createShader(gl[type]);
			gl.shaderSource(ret, code);
			gl.compileShader(ret);
			if (!gl.getShaderParameter(ret, gl.COMPILE_STATUS))
				throw gl.getShaderInfoLog(ret);
			ret.refCount = 1;
			ret.release = function () { if (--ret.refCount) gl.deleteShader(ret); };
			return ret;
		}
		gl.CreateProgram = function (shaders) {
			let ret = gl.createProgram();
			shaders.forEach(c => { gl.attachShader(ret, c); });
			gl.linkProgram(ret);
			if (!gl.getProgramParameter(ret, gl.LINK_STATUS)) 
				throw gl.getProgramInfoLog(ret);
			shaders.forEach(c => { gl.detachShader(ret, c); c.release(); });
			ret.refCount = 1;
			ret.release = function () { if (--ret.refCount) gl.deleteProgram(ret); };
			return ret;
		}
		gl.CreateProgram2 = ((shaderCode) => {
			function getUniforms(p, code, ret) {
				code = code.replaceAll(/\/\*(.|\n)*\*\//g, "").replaceAll(/\/\/.*/g, "").replace(/\s+/g, " ").trim();
				let m = Array.from(code.matchAll(/uniform\s([^\s]+)\s([^;]+);/g)).map(c => ({ type: c[1], args: c[2].split(",").map(d => d.trim()) }))
				m.forEach(c => {
					c.args.forEach(a => {
						let loc = gl.getUniformLocation(p, a);
						if (loc) {
							switch (c.type) {
								case "float": ret[a] = gl.uniform1fv.bind(gl, loc); break;
								case "vec2": ret[a] = gl.uniform2fv.bind(gl, loc); break;
								case "vec3": ret[a] = gl.uniform3fv.bind(gl, loc); break;
								case "vec4": ret[a] = gl.uniform4fv.bind(gl, loc); break;
								case "mat3": ret[a] = gl.uniformMatrix3fv.bind(gl, loc, false); break;
								case "mat4": ret[a] = gl.uniformMatrix4fv.bind(gl, loc, false); break;
								default: return;
							}
						}
					});
				});
			}
			let p = gl.CreateProgram(shaderCode.map(gl.CreateShader));
			let uniforms = {};
			shaderCode.forEach(c => getUniforms(p, c.code, uniforms));
			p.uniforms = Object.keys(uniforms).map(k => ({ name: k, fnc: uniforms[k] }));
			return p;
		})
		gl.createVertexBuffer = function (array) {
			let ret = gl.createBuffer();
			gl.bindBuffer(gl.ARRAY_BUFFER, ret);
			gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(array), gl.STATIC_DRAW);
			ret.refCount = 1;
			ret.release = function () { if (--ret.refCount) gl.deleteBuffer(ret); };
			return ret;
		}
		gl.createIndexBuffer = function (array) {
			let ret = gl.createBuffer();
			gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ret);
			gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint32Array(array), gl.STATIC_DRAW);
			ret.length = array.length;
			ret.refCount = 1;
			ret.release = function () { 
				if (--ret.refCount) 
				gl.deleteBuffer(ret); 
			};
			return ret;
		}
		gl.CreateTexture2D = function (img) {
			let ret = gl.createTexture();
			ret.refCount = 1;
			ret.release = function () { if (--ret.refCount) gl.deleteTexture(ret); };
			gl.bindTexture(gl.TEXTURE_2D, ret);
			gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
			gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, img);
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
			gl.bindTexture(gl.TEXTURE_2D, null);
			return ret;
		}
		var ImagePromises = {};
		gl.LoadTexture = function (imgName) {
			if (ImagePromises[imgName]) return new Promise(r => {
				ImagePromises[imgName].refCount++;
				r(ImagePromises[imgName])
			});
			return PreloadImage(imgName).then(img => ImagePromises[imgName] = gl.CreateTexture2D(img));
		}
		gl.buildAttributes = function (program, attr) {
			let stride = 0
			let a = attr.map(a => {
				let sz = attrSize(a);
				let off = stride;
				stride += sz;
				return {
					location: gl.getAttribLocation(program, a.name),
					type: gl[a.type],
					count: a.size,
					offset: off
				}
			});
			return {
				attributes: a,
				stride: stride
			};
		}
		gl.mesh = function(m){
			return {
				enabled: true,
				bindings: {},
				render: function (x) {
					if (!this.enabled) return;
					x = {
						...this.bindings,
						...(x || {})
					};
					gl.useProgram(this.program);
					gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
					gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.index);
					this.attributes.forEach(a => {
						gl.enableVertexAttribArray(a.location);
						gl.vertexAttribPointer(a.location, a.count, a.type, false, this.stride, a.offset);
					});
					this.textures && this.textures.forEach((t, i) => {
						gl.activeTexture(gl.TEXTURE0 + i);
						gl.bindTexture(gl.TEXTURE_2D, t.texture);
						gl.uniform1i(gl.getUniformLocation(this.program, t.sampler), i);
					});
					this.program.uniforms.forEach(u => u.fnc(x[u.name]));
					gl.drawElements(this.primitive, this.index.length, gl.UNSIGNED_INT, 0);
				},
				release: function () {
					this.vertexBuffer && this.vertexBuffer.release();
					this.index && this.index.release();
					this.program && this.program.release();
					this.textures && this.textures.forEach(t => t.release());
				},
				set Bindings(data) {
					this.bindings = {
						...this.bindings,
						...(data || {})
					}
				},
				set Vertices(data) {
					this.vertexBuffer && this.vertexBuffer.release();
					this.vertexBuffer = gl.createVertexBuffer([].concat(...data));
				},
				set Indices(data) {
					this.index && this.index.release();
					this.index = gl.createIndexBuffer([].concat(...data));
				},
				...m
			}
		}
		gl.BuildMesh = function (mesh, bindings={}) {
			let p = gl.CreateProgram2(mesh.shaders);
			let ret = gl.mesh({
				program: p,
				primitive: gl[mesh.primitive],
				vertexBuffer: gl.createVertexBuffer([].concat(...mesh.vertices)),
				index: gl.createIndexBuffer([].concat(...mesh.indices)),
				...gl.buildAttributes(p, mesh.attributes),
				bindings: bindings
			});
			if (mesh.textures && mesh.textures.length) {
				return Promise.all(mesh.textures.map(c => gl.LoadTexture(c.file))).then(r => {
					ret.textures = r.map((c, i) => ({ sampler: mesh.textures[i].sampler, texture: c }));
					return ret;
				});
			} else {
				ret.textures = [];
				return ret;
			}
		}
		gl.FetchMesh = function (src, bindings) {
			return fetch(src).then(c => c.text()).then(c => gl.BuildMesh((eval("(()=>{return " + c + ";})"))(), bindings));
		}
	}
	gl.Canvas = canvas;
	{
		canvas.requestPointerLock = canvas.requestPointerLock || canvas.mozRequestPointerLock;
		document.exitPointerLock = document.exitPointerLock || document.mozExitPointerLock;

		canvas.onmousedown = e => {
			e.preventDefault();
			e.relOffsetX = (e.offsetX / e.target.offsetWidth - 0.5) * -2;
			e.relOffsetY = (e.offsetY / e.target.offsetHeight - 0.5) * 2;
			if (e.buttons & 2 && gl.onrelativemove && document.pointerLockElement != canvas && document.mozPointerLockElement != canvas)
				canvas.requestPointerLock();
			else if (gl.onmousedown) gl.onmousedown(e);
		}
		canvas.onmouseup = e => {
			e.preventDefault();
			e.relOffsetX = (e.offsetX / e.target.offsetWidth - 0.5) * -2;
			e.relOffsetY = (e.offsetY / e.target.offsetHeight - 0.5) * 2;
			if (!(e.buttons & 2) && gl.onrelativemove && (document.pointerLockElement === canvas || document.mozPointerLockElement === canvas))
				document.exitPointerLock();
			else if (gl.onmouseup) gl.onmouseup(e);
		}

		canvas.onmousemove = e => {
			e.preventDefault();
			e.relOffsetX = (e.offsetX / e.target.offsetWidth - 0.5) * -2;
			e.relOffsetY = (e.offsetY / e.target.offsetHeight - 0.5) * 2;
			if (gl.onrelativemove && (document.pointerLockElement === canvas || document.mozPointerLockElement === canvas))
				gl.onrelativemove(e.movementX / canvas.clientWidth, e.movementY / canvas.clientWidth, e.buttons);
			else if (gl.onmousemove) gl.onmousemove(e);
		}
		canvas.addEventListener('contextmenu', e => {
			e.preventDefault();
		});
		/*canvas.oncontextmenu = (e => {
			e.preventDefault();
			e.stopPropagation();
		});*/
	}
	return gl;
}

function ObjectBuilder(){
	const HasNormal = 2;
	const HasColor = 4;
	const HasTex = 1;
	const HasTex3D = 8;

	this.GL_POINTS = 0;
	this.GL_LINES = 1;
	this.GL_LINE_LOOP = 2;
	this.GL_LINE_STRIP = 3;
	this.GL_TRIANGLES = 4;
	this.GL_TRIANGLE_STRIP = 5;
	this.GL_TRIANGLE_FAN = 6;
	this.GL_QUADS = 7;
	this.GL_QUAD_STRIP = 8;
	this.GL_POLYGON = 9;
	this.GL_TRIANGLE_STRIP = 10;
	//this.GL_TRIANGLE_STRIP = 11;
	//this.GL_TRIANGLE_STRIP = 12;
	this.GL_QUAD_STRIP = 13;
	this.GL_LINE_LOOP = 14;
	this.GL_LINE_STRIP = 15;

	function AddQuad(data, Start) {
		let i = data.Vertex.length - 4;
		data.Index.push(i + 0);
		data.Index.push(i + 1);
		data.Index.push(i + 2);

		data.Index.push(i + 0);
		data.Index.push(i + 2);
		data.Index.push(i + 3);
	}
	function AddQuad2(data, Start) {
		let i = data.Vertex.length - 4;
		data.Index.push(i + 1);
		data.Index.push(i + 0);
		data.Index.push(i + 2);

		data.Index.push(i + 1);
		data.Index.push(i + 2);
		data.Index.push(i + 3);
	}

	function AddTriangleFan(data, Start) {
		let i = data.Vertex.length - 2;
		data.Index.push(Start);
		data.Index.push(i + 0);
		data.Index.push(i + 1);
	}

	function AddTriangle(data, Start) {
		let i = data.Vertex.length - 1;
		data.Index.push(i - 2);
		data.Index.push(i - 1);
		data.Index.push(i - 0);
	}
	function AddTriangleStrip1(data, Start) {
		let i = data.Vertex.length - 1;
		data.Index.push(i - 3);
		data.Index.push(i - 1);
		data.Index.push(i - 0);
	}
	function AddTriangleStrip2(data, Start) {
		let i = data.Vertex.length - 1;
		data.Index.push(i - 1);
		data.Index.push(i - 2);
		data.Index.push(i - 0);
	}
	function AddPoint(data, Start) {
		let i = data.Vertex.length - 1;
		data.Index.push(i);
	}
	function AddLine(data, Start) {
		let i = data.Vertex.length - 1;
		data.Index.push(i - 1);
		data.Index.push(i);
	}
	function AddLineStrip(data, Start) {
		let i = data.Vertex.length - 1;
		data.Index.push(data.Index.back());
		data.Index.push(i);
	}
	function AddLineLoop(data, Start) {
		let i = data.Vertex.length - 1;
		data.Index[data.Index.length - 2] = i;
		data.Index[data.Index.length - 1] = i + 1;
		data.Index.push(i + 1);
		data.Index.push(Start);
	}
	const AddLookup = [
		{ MakePrimitive: AddPoint, Min: 0, Max: 1, Next: 0, Mode: "POINTS" },
		{ MakePrimitive: AddLine, Min: 0, Max: 2, Next: 1, Mode: "LINES" },
		{ MakePrimitive: AddLine, Min: 1, Max: 2, Next: 14, Mode: "LINES" },
		{ MakePrimitive: AddLine, Min: 1, Max: 2, Next: 15, Mode: "LINES" },
		{ MakePrimitive: AddTriangle, Min: 0, Max: 3, Next: 4, Mode: "TRIANGLES" },
		{ MakePrimitive: AddTriangle, Min: 2, Max: 3, Next: 10, Mode: "TRIANGLES" },
		{ MakePrimitive: AddTriangleFan, Min: 2, Max: 3, Next: 6, Mode: "TRIANGLES" },
		{ MakePrimitive: AddQuad, Min: 0, Max: 4, Next: 7, Mode: "TRIANGLES" },
		{ MakePrimitive: AddQuad, Min: 2, Max: 4, Next: 13, Mode: "TRIANGLES" },
		{ MakePrimitive: null, Min: 0, Max: 0x7fffffff, Next: 9, Mode: "TRIANGLES" },
		{ MakePrimitive: AddTriangleStrip1, Min: 2, Max: 3, Next: 11, Mode: "TRIANGLES" },
		{ MakePrimitive: AddTriangleStrip2, Min: 2, Max: 3, Next: 12, Mode: "TRIANGLES" },
		{ MakePrimitive: AddTriangle, Min: 2, Max: 3, Next: 11, Mode: "TRIANGLES" },
		{ MakePrimitive: AddQuad, Min: 2, Max: 4, Next: 11, Mode: "TRIANGLES" },
		{ MakePrimitive: AddLineLoop, Min: 1, Max: 2, Next: 2, Mode: "LINES" },
		{ MakePrimitive: AddLineStrip, Min: 1, Max: 2, Next: 3, Mode: "LINES" },
	]

	let Data = {
		Color: [],
		Vertex: [],
		Normal: [],
		TexCoord: [],
		TexCoord3D: [],
		Index: []
	};
	let Current = {
		Color: [0, 0, 0, 0],
		Normal: [0, 0, 0],
		TexCoord: [0, 0],
		TexCoord3D: [0, 0, 0]
	}
	let MakePrimitive = null;
	let Flags = 0, Mode = 0, Count = 0, Max = 0, Min, Start;

	this.Color = function (v) {
		Flags |= HasColor;
		Current.Color = v;
	}
	this.Color = function (v) {
		Flags |= HasColor;
		Current.Color = [v[0], v[1], v[2], v[3] || 1.0];
	}
	this.Vertex = function (v) {
		Data.Vertex.push(v);
		Data.Color.push(Current.Color);
		Data.Normal.push(Current.Normal);
		Data.TexCoord.push(Current.TexCoord);
		Data.TexCoord3D.push(Current.TexCoord3D);
		Count++;

		if (Count == Max) {
			let mode = AddLookup[Mode];
			mode.MakePrimitive(Data, Start);
			Mode = mode.Next;
			Count = Min;
		}
	}
	this.Normal = function (v) {
		Flags |= HasNormal;
		Current.Normal = v;
	}
	this.TexCoord = function (v) {
		if(v.length==2){
			Flags |= HasTex;
			Current.TexCoord = v;
		} else {
			Flags |= HasTex3D;
			Current.TexCoord3D = v;
		}
	}

	this.Begin = function (p) {
		Mode = p;
		let mode = AddLookup[Mode];
		Min = mode.Min;
		Max = mode.Max;
		Count = 0;
		Start = Data.Vertex.length;
	}
	this.End = function() {
		if (Max != 0x7fffffff && Count != Min) {
			Count = Data.Vertex.length - Count + Min;
			Data.Vertex.length = (Count);
			Data.Color.length = (Count);
			Data.Normal.length = (Count);
			Data.TexCoord.length = (Count);
			Data.TexCoord3D.length = (Count);
		}
	}
	this.Reset = function() {
		Flags = Mode = Count = Max = 0;
		Current = {
			Color: [0, 0, 0, 0],
			Normal: [0, 0, 0],
			TexCoord: [0, 0],
			TexCoord3D: [0, 0, 0]
		}
		Data.Vertex = [];
		Data.Color = [];
		Data.Normal = [];
		Data.TexCoord = [];
		Data.TexCoord3D = [];
	}
	function Hash(o) {
		if(!o) return 0;
		let s = new Uint8Array((new Float32Array(o)).buffer);
		let hash = 0;
		for (i = 0; i < s.length; i++) {
			hash = ((hash << 5) - hash) + s[i];
			hash = hash & 0x7fffffff;
		}
		return hash;
	}
	function eq(a, b){
		return !a.find((c,i) => c!=b[i]);
	}
	function Interleave(out, offset, data){
		let outStride = out.length / data.length;
		data.forEach(d=>{
			d.forEach((c, i) =>{
				out[offset + i] = c;
			});
			offset += outStride;
		})
	}

	this.Build = function(){
		let ret = {
			primitive: AddLookup[Mode].Mode,
			attributes: [],
			indices: [],
			vertices: []
		}
		let sz = Data.Vertex.length;
		let Stride = (3 +
			3 * !!(Flags & HasNormal) +
			2 * !!(Flags & HasTex) +
			3 * !!(Flags & HasTex3D) +
			4 * !!(Flags & HasColor))
		let dsz = sz * Stride;
		let tmp = null;

		ret.attributes.push({ name: "Position", type: "FLOAT", size: 3 });
		if(Stride==3)
			tmp = new Float32Array([].concat(...Data.Vertex));
		else{
			tmp = new Float32Array(dsz);
			let off = 0;
			Interleave(tmp, off, Data.Vertex); off += 3;

			if(Flags & HasNormal){
				ret.attributes.push({ name: "Normal", type: "FLOAT", size: 3 });
				Interleave(tmp, off, Data.Normal); off += 3;
			}

			if (Flags & HasTex) {
				ret.attributes.push({ name: "UV", type: "FLOAT", size: 2 });
				Interleave(tmp, off, Data.TexCoord); off += 2;
			}

			if (Flags & HasTex3D) {
				ret.attributes.push({ name: "UVW", type: "FLOAT", size: 3 });
				Interleave(tmp, off, Data.TexCoord3D); off += 3;
			}

			if (Flags & HasColor) {
				ret.attributes.push({ name: "Color", type: "FLOAT", size: 4 });
				Interleave(tmp, off, Data.Color); off += 4;
			}
		}
		{
			function element(at){
				let ret = [];
				for(let i = 0; i < Stride; i++) ret.push(tmp[at * Stride + i]);
				return ret;
			}
			let HashMap ={};
			let count = 0;
			
			Data.Index.forEach(i => {
				let dt = element(i);
				let h = Hash(dt) + "";
				HashMap[h] = HashMap[h] || [];
				let similar = HashMap[h];
				let ss = similar.find(s => eq(dt, ret.vertices[s]));
				if (ss !== null && ss !== undefined) {
					ret.indices.push(ss);
				} else {
					ret.vertices.push(dt);
					ret.indices.push(count);
					similar.push(count++);
				}
			})
		}

		return ret;
	}
}

function MakeSphereUV(segH, segV){
	let Verts = Array.from({ length: ((segV - 1) * (segH + 1) + 2)});
	Verts[0] = { pos: [0, 1, 0], UV: [0.5, 1] };
	Verts[1] = { pos: [0, -1, 0], UV: [0.5, 0] };

	const step1 = Math.PI / segV;
	const step2 = 2 * Math.PI / segH;

	let curV = step1;
	let curU = 0;
	let k = 2;
	for(let i = 1; i < segV; i++, k++){
		curU = 0;
		let r =Math.sin(curV);
		let y = Math.cos(curV);
		let v = 1.0 - i * (1.0 / segV);
		for(let j = 0; j < segH; j++, k++){
			Verts[k] = { pos: [r * Math.cos(curU), y, r * Math.sin(curU)], UV: [j / (1.0 * segH), v] };
			curU += step2;
		}
		Verts[k] = { pos: [r, y, 0], UV: [1, v] };
		curV += step1;
	}
	let gl = new ObjectBuilder();
	gl.Begin(gl.GL_TRIANGLES);
	for (let i = 0; i < segH; i++) {
		let v1 = Verts[0];
		let v2 = Verts[i + 2];
		let v3 = Verts[i + 3];

		let v4 = Verts[i + 2 + (segH + 1) * (segV - 2)];
		let v5 = Verts[i + 3 + (segH + 1) * (segV - 2)];
		let v6 = Verts[1];

		gl.TexCoord([ 0.5 / segH + i / (1.0 * segH), 1 ]);
		gl.Vertex(v1.pos);

		gl.TexCoord(v2.UV);
		gl.Vertex(v2.pos);

		gl.TexCoord(v3.UV);
		gl.Vertex(v3.pos);



		gl.TexCoord(v4.UV);
		gl.Vertex(v4.pos);

		gl.TexCoord(v5.UV);
		gl.Vertex(v5.pos);

		gl.TexCoord([ 0.5 / segH + i / (1.0 * segH), 0 ]);
		gl.Vertex(v6.pos);
	}

	for (let i = 0; i < segV - 2; i++) {
		for (let j = 0; j < segH; j++) {
			let v1 = Verts[2 + i * (segH + 1) + j];
			let v2 = Verts[2 + i * (segH + 1) + j + 1];
			let v3 = Verts[2 + i * (segH + 1) + j + (segH + 1)];
			let v4 = Verts[2 + i * (segH + 1) + j + (segH + 1)];
			let v5 = Verts[2 + i * (segH + 1) + j + 1];
			let v6 = Verts[2 + i * (segH + 1) + j + 1 + (segH + 1)];

			gl.TexCoord(v1.UV);
			gl.Vertex(v1.pos);

			gl.TexCoord(v2.UV);
			gl.Vertex(v2.pos);

			gl.TexCoord(v3.UV);
			gl.Vertex(v3.pos);


			gl.TexCoord(v4.UV);
			gl.Vertex(v4.pos);

			gl.TexCoord(v5.UV);
			gl.Vertex(v5.pos);

			gl.TexCoord(v6.UV);
			gl.Vertex(v6.pos);
		}
	}
	gl.End();
	return gl.Build();
}