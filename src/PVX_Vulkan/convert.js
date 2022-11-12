(function(){
	let functions = JSON.parse(ReadUtf("vulkan.json"))
	let txt = ReadUtf("PVX_Vulkan.cpp");
	let m = txt.match(/.*\/\/\s+Auto Generated\s+\-\s+Start.*/);
	let b = txt.substr(0, m.index + m[0].length);
	let e = txt.substr(txt.search(/.*\/\/\s+Auto Generated\s+\-\s+End.*/));

	return (b + "\n\n" + functions.map(c => `	Vk${c.name} Create${c.name}(${c.params?c.params.map(({type, name})=>`${type} ${name}`).join(", "):""}){
    	Vk${c.name}CreateInfo createInfo{};
    	createInfo.sType = VK_STRUCTURE_TYPE_${c.name.toUpperCase()}_CREATE_INFO;
    	createInfo.pNext = nullptr;${c.options?Object.keys(c.options).map(k => `
    	createInfo.${k} = ${c.options[k]};`).join("\n"):""}${c.params?c.params.map(({ name }) => `
    	createInfo.${name} = ${name};`).join("\n"):""}
    
    	Vk${c.name} object;
    	vkCreate${c.name}(&createInfo, nullptr, &object);
    	return object;
    }`).join("\n\n") + "\n\n" + e).replace(/\r*\n/g, "\r\n").replace(/\s+/g, c => c.replace(/    /g, "\t"));
})()