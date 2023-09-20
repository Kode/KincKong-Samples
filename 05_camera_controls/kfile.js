const project = new Project('Example');

await project.addProject('../Kinc', {kong: true});

project.addFile('Sources/**');
project.addKongDir('Shaders');
project.setDebugDir('Deployment');

project.flatten();

resolve(project);
