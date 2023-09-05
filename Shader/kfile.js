const project = new Project('ShaderTest', {kong: true});

await project.addProject('../Kinc');

project.addFile('Sources/**');
project.addKongDir('Shaders');
project.setDebugDir('Deployment');

project.flatten();

resolve(project);
